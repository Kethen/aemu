/*
 * This file is part of PRO ONLINE.

 * PRO ONLINE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO ONLINE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO ONLINE. If not, see <http://www.gnu.org/licenses/ .
 */

#include "../../common.h"

// probably over provisioning
GamemodeInternal *_gamemode_replicas[256] = {0};
int largest_gamemode_replica = 0;
void *replica_receive_buffer = NULL;
int _gamemode_replica_stop_thread = 0;
SceUID _gamemode_replica_thread_id = -1;

static int gamemode_replica_thread(SceSize args, void *argp)
{
	printk("%s: gamemode replica thread started\n", __func__);
	sceKernelDelayThread(GAMEMODE_INIT_DELAY_USEC);

	while(!_gamemode_replica_stop_thread)
	{
		int recv_status = 0;
		int packet_limit = 20;
		while(recv_status == 0 && packet_limit > 0 && !_gamemode_replica_stop_thread)
		{
			if (sceKernelTryLockLwMutex(&_gamemode_lock, 1) != 0)
			{
				sceKernelDelayThread(GAMEMODE_UPDATE_INTERVAL_USEC);
				continue;
			}

			#define UNLOCK_CONTINUE() { \
				sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
				continue; \
			}

			SceNetEtherAddr saddr = {0};
			uint16_t sport = 0;
			int len = largest_gamemode_replica;
			packet_limit--;

			#define NON_BLOCK_RECV 1

			recv_status = sceNetAdhocPdpRecv(_gamemode_socket, &saddr, &sport, replica_receive_buffer, &len, 1000000, NON_BLOCK_RECV);

			#if NON_BLOCK_RECV
			if (recv_status == ADHOC_WOULD_BLOCK)
			{
				//printk("%s: no packet in buffer\n", __func__);
				UNLOCK_CONTINUE();
			}

			#else

			if (recv_status == ADHOC_TIMEOUT)
			{
				printk("%s: packet receive timed out\n", __func__);
				UNLOCK_CONTINUE();
			}
			#endif

			if (recv_status != 0)
			{
				printk("%s: failed receiving packet, 0x%x\n", __func__, recv_status);
				UNLOCK_CONTINUE();
			}

			#ifdef DEBUG
			int packet_processed = 0;
			#endif

			for (int i = 0;i < sizeof(_gamemode_replicas) / sizeof(GamemodeInternal *);i++)
			{
				if (_gamemode_replicas[i] == NULL)
				{
					continue;
				}
				if (!_isMacMatch(&_gamemode_replicas[i]->saddr, &saddr))
				{
					continue;
				}

				#ifdef DEBUG
				packet_processed = 1;
				#endif

				if (_gamemode_replicas[i]->data_size != len)
				{
					printk("%s: received data has bad length, expected %u, got %d\n", __func__, _gamemode_replicas[i]->data_size, len);
					continue;
				}

				// usable data
				_gamemode_replicas[i]->last_recv = sceKernelGetSystemTimeWide();
				_gamemode_replicas[i]->data_updated = 1;
				memcpy(_gamemode_replicas[i]->recv_buf, replica_receive_buffer, len);
			}

			#ifdef DEBUG
			if (!packet_processed)
			{
				//printk("%s: received data has no handler\n", __func__);
			}
			#endif

			UNLOCK_CONTINUE();
		}
		sceKernelDelayThread(GAMEMODE_UPDATE_INTERVAL_USEC);
	}

	printk("%s: gamemode replica thread stopped\n", __func__);
	_gamemode_replica_stop_thread = -1;
	return 0;
}

/**
 * Adhoc Emulator Gamemode Peer Replica Buffer Creator
 * @param src Peer MAC Address
 * @param ptr Buffer Address
 * @param size Buffer Size (in Bytes)
 * @return Replica ID >= 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_DATALEN, ADHOC_NOT_IN_GAMEMODE, ADHOC_ALREADY_CREATED, NET_NO_SPACE
 */
int proNetAdhocGameModeCreateReplica(const SceNetEtherAddr * src, void * ptr, uint32_t size)
{
	sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
		printk("%s: 0x%x 0x%x %u, %d\n", __func__, src, ptr, size, _v); \
		return _v; \
	}

	if (!_init)
	{
		RETURN_UNLOCK(ADHOC_NOT_INITIALIZED);
	}

	if (src == NULL || ptr == NULL)
	{
		RETURN_UNLOCK(ADHOC_INVALID_ARG);
	}

	// Check if we're in game mode
	SceNetAdhocctlGameModeInfo gamemode_info;
	int gamemode_info_get_status = sceNetAdhocctlGetGameModeInfo(&gamemode_info);
	if (gamemode_info_get_status != 0)
	{
		RETURN_UNLOCK(ADHOC_NOT_IN_GAMEMODE);
	}

	int next_empty_slot = -1;

	// Find the next slot and check for existing replication
	for(int i = 0;i < sizeof(_gamemode_replicas) / sizeof(GamemodeInternal *);i++){
		if (_gamemode_replicas[i] != NULL)
		{
			if (_isMacMatch(&(_gamemode_replicas[i]->saddr), src))
			{
				//RETURN_UNLOCK(ADHOC_ALREADY_CREATED);
				// PPSSPP seems to return the id instead, while not updating the address..?
				RETURN_UNLOCK(i + 1);
			}
		}
		else if (next_empty_slot == -1)
		{
			next_empty_slot = i;
		}
	}

	if (next_empty_slot == -1)
	{
		// No slot, damn
		printk("%s: we ran out of slots, how\n", __func__);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	// Allocate memory
	GamemodeInternal *gamemode = (GamemodeInternal *)malloc(sizeof(GamemodeInternal) + size);
	if (gamemode == NULL)
	{
		// Out of memory
		printk("%s: out of heap\n", __func__);
		RETURN_UNLOCK(NET_NO_SPACE);
	}
	memset(gamemode, 0, sizeof(GamemodeInternal));
	gamemode->recv_buf = ((void *)gamemode) + sizeof(GamemodeInternal);
	printk("%s: gamemode 0x%x size 0x%x recv_buf 0x%x\n", __func__, gamemode, sizeof(GamemodeInternal), gamemode->recv_buf);


	if (size > largest_gamemode_replica)
	{
		void *new_buffer = malloc(size);
		if (new_buffer == NULL)
		{
			printk("%s: failed allocating receive buffer\n", __func__);
			free(gamemode);
			RETURN_UNLOCK(NET_NO_SPACE);
		}
		largest_gamemode_replica = size;
		if (replica_receive_buffer != NULL)
		{
			free(replica_receive_buffer);
		}
		replica_receive_buffer = new_buffer;
		printk("%s: new receive buffer 0x%x with size %d\n", __func__, replica_receive_buffer, size);
	}

	gamemode->saddr = *src;
	gamemode->data = ptr;
	gamemode->data_size = size;
	gamemode->data_updated = 0;

	if (_gamemode_socket_users == 0)
	{
		SceNetEtherAddr local_mac = {0};
		sceNetGetLocalEtherAddr(&local_mac);

		// XXX the size here would be wrong for shared socket, we're just riding the fact that inet sockets don't have a recv buffer size setting
		_gamemode_socket = sceNetAdhocPdpCreate(&local_mac, ADHOC_GAMEMODE_PORT, size, 0);

		if (_gamemode_socket < 0)
		{
			printk("%s: failed creating gamemode socket, 0x%x\n", __func__, _gamemode_socket);
			free(gamemode);
			RETURN_UNLOCK(NET_NO_SPACE);
		}
	}

	if (_gamemode_replica_thread_id < 0)
	{
		_gamemode_replica_thread_id = sceKernelCreateThread("replica receive thread", gamemode_replica_thread, 111, 1024 * 8, 0, NULL);

		if (_gamemode_replica_thread_id < 0)
		{
			printk("%s: failed creating receive thread, 0x%x\n", __func__, _gamemode_replica_thread_id);
			if (_gamemode_socket_users == 0)
			{
				sceNetAdhocPdpDelete(_gamemode_socket, 0);
				_gamemode_socket = -1;
			}
			free(gamemode);
			RETURN_UNLOCK(NET_NO_SPACE);
		}

		_gamemode_replica_stop_thread = 0;
		int thread_start_status = sceKernelStartThread(_gamemode_replica_thread_id, 0, NULL);
		if (thread_start_status < 0)
		{
			// can it fail?
			printk("%s: failed starting receive thread, 0x%x\n", __func__, thread_start_status);

			int thread_delete_status = sceKernelDeleteThread(_gamemode_replica_thread_id);
			if (thread_delete_status < 0)
			{
				// uhhhh
				printk("%s: failed removing receive thread, 0x%x\n", __func__, thread_delete_status);
			}
			_gamemode_replica_thread_id = -1;

			if (_gamemode_socket_users == 0)
			{
				sceNetAdhocPdpDelete(_gamemode_socket, 0);
				_gamemode_socket = -1;
			}
			free(gamemode);
			RETURN_UNLOCK(NET_NO_SPACE);
		}
	}

	// replica created
	_gamemode_replicas[next_empty_slot] = gamemode;
	_gamemode_socket_users++;

	sceKernelUnlockLwMutex(&_gamemode_lock, 1);

	#if 1
	// Wait for the first replication
	uint64_t begin = sceKernelGetSystemTimeWide();
	while (!gamemode->data_updated && sceKernelGetSystemTimeWide() - begin < GAMEMODE_SYNC_TIMEOUT_USEC)
	{
		sceKernelDelayThread(50000);
	}
	#endif

	if (_gamemode_socket_users - 1 == _gamemode_num_peers){
		// PPSSPP blocks here
		while(!_gamemode_all_received){
			sceKernelDelayThread(1000);
		}
	}

	return next_empty_slot + 1;
}
