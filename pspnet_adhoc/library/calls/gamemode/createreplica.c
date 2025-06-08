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

static int gamemode_replica_thread(SceSize args, void *argp)
{
	GamemodeInternal *gamemode = (GamemodeInternal *)argp;
	sceKernelDelayThread(GAMEMODE_INIT_DELAY_USEC);
	while(!gamemode->stop_thread)
	{
		sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
		#define UNLOCK_DELAY_CONTINUE(_d) { \
			sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
			sceKernelDelayThread(_d); \
			continue; \
		}

		SceNetEtherAddr saddr = {0};
		uint16_t sport = 0;
		int len = gamemode->data_size;
		int recv_status = sceNetAdhocPdpRecv(gamemode->pdp_sock_id, &saddr, &sport, gamemode->recv_buf, &len, 0, 1);

		if (recv_status == ADHOC_WOULD_BLOCK)
		{
			printk("%s: no packet in buffer\n", __func__);
			UNLOCK_DELAY_CONTINUE(GAMEMODE_UPDATE_INTERVAL_USEC);
		}

		if (recv_status != 0)
		{
			printk("%s: failed receiving packet, 0x%x\n", __func__, recv_status);
			UNLOCK_DELAY_CONTINUE(1000);
		}

		if (gamemode->data_size != len)
		{
			printk("%s: received data has bad length, expected %u, got %d\n", __func__, gamemode->data_size, len);
			UNLOCK_DELAY_CONTINUE(1000);
		}

		if (!_isMacMatch(&gamemode->saddr, &saddr))
		{
			printk("%s: received data from non host\n", __func__);
			UNLOCK_DELAY_CONTINUE(1000);
		}

		// XXX should we check sport?

		// We have new data in recv_buf
		gamemode->last_recv = sceKernelGetSystemTimeWide();
		gamemode->data_updated = 1;
		UNLOCK_DELAY_CONTINUE(GAMEMODE_UPDATE_INTERVAL_USEC);
	}

	gamemode->stop_thread = -1;
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
		printk("%s: 0x%x 0x%x %u, %d", __func__, src, ptr, size, _v); \
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
		printk("%s: we went out of slots, how\n", __func__);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	// Allocate memory
	GamemodeInternal *gamemode = (GamemodeInternal *)malloc(sizeof(GamemodeInternal));
	if (gamemode == NULL)
	{
		// Out of memory
		printk("%s: out of heap\n", __func__);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	gamemode->recv_buf = malloc(size);
	if (gamemode->recv_buf == NULL)
	{
		printk("%s: failed allocating receive buffer\n");
		free(gamemode);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	gamemode->saddr = *src;
	gamemode->data = ptr;
	gamemode->data_size = size;
	gamemode->data_updated = 0;
	gamemode->pdp_sock_id = sceNetAdhocPdpCreate(&(gamemode->saddr), ADHOC_GAMEMODE_PORT, size, 0);

	if (gamemode->pdp_sock_id < 0)
	{
		printk("%s: failed creating pdp recv socket, 0x%x\n", __func__, gamemode->pdp_sock_id);
		free(gamemode->recv_buf);
		free(gamemode);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	gamemode->thread_id = sceKernelCreateThread("replica receive thread", gamemode_replica_thread, 111, 1024 * 4, 0, NULL);
	gamemode->stop_thread = 0;

	if (gamemode->thread_id < 0)
	{
		printk("%s: failed creating receive thread, 0x%x\n", __func__, gamemode->thread_id);
		sceNetAdhocPdpDelete(gamemode->pdp_sock_id, 0);
		free(gamemode->recv_buf);
		free(gamemode);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	int thread_start_status = sceKernelStartThread(gamemode->thread_id, 0, NULL);
	if (thread_start_status < 0)
	{
		// can it fail?
		printk("%s: failed starting receive thread, 0x%x\n", __func__, thread_start_status);
		sceNetAdhocPdpDelete(gamemode->pdp_sock_id, 0);
		free(gamemode->recv_buf);
		free(gamemode);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	// replica created;
	_gamemode_replicas[next_empty_slot] = gamemode;
	RETURN_UNLOCK(next_empty_slot + 1);
}
