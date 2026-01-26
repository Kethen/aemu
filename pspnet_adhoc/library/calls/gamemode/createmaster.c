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

static int gamemode_info_same(SceNetAdhocctlGameModeInfo *lhs, SceNetAdhocctlGameModeInfo *rhs)
{
	if (lhs->num != rhs->num)
	{
		return 0;
	}

	for(int i = 0;i < lhs->num;i++)
	{
		if (!_isMacMatch(&lhs->member[i], &rhs->member[i]))
		{
			return 0;
		}
	}

	return 1;
}

GamemodeInternal _gamemode = {0};
SceLwMutexWorkarea _gamemode_lock = {0};
const SceNetEtherAddr _broadcast_mac = {.data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
SceUID _gamemode_socket = -1;
int _gamemode_socket_users = 0;
SceUID _gamemode_thread_id = -1;
int _gamemode_stop_thread = 0;
int _gamemode_num_peers = 0;
int _gamemode_all_received = 0;

static int gamemode_master_thread(SceSize args, void *argp)
{
	printk("%s: gamemode master thread started\n", __func__);
	sceKernelDelayThread(GAMEMODE_INIT_DELAY_USEC);

	SceNetAdhocctlGameModeInfo last_gamemode_info = {0};

	uint64_t begin = sceKernelGetSystemTimeWide();

	while(!_gamemode_stop_thread)
	{
		if(sceKernelTryLockLwMutex(&_gamemode_lock, 1) != 0)
		{
			sceKernelDelayThread(GAMEMODE_UPDATE_INTERVAL_USEC);
			continue;
		}

		// yolo braodcast
		// in the PPSSPP implementation, adhoc ctl should update gamemode peer status, as more peers are inserted
		// packets should only send to changed peers, but let's see if this works

		// check for peer change since last time
		SceNetAdhocctlGameModeInfo gamemode_info = {0};
		int gamemode_info_get_status = sceNetAdhocctlGetGameModeInfo(&gamemode_info);
		if (gamemode_info_get_status != 0)
		{
			//printk("%s: failed grabbing gamemode info for some reason, 0x%x\n", __func__, gamemode_info_get_status);
			//sceKernelDelayThread(100000);
			//continue;
		}

		if (_gamemode.data_updated || !gamemode_info_same(&last_gamemode_info, &gamemode_info))
		{
			// Peer changes or data is updated, broadcast
			last_gamemode_info = gamemode_info;
			#define NON_BLOCK_SEND 1
			sceNetAdhocPdpSend(_gamemode_socket, &_broadcast_mac, ADHOC_GAMEMODE_PORT, _gamemode.recv_buf, _gamemode.data_size, 1000000, NON_BLOCK_SEND);
			if (_gamemode.data_updated)
			{
				_gamemode.data_updated = 0;
			}
		}

		if (!_gamemode_all_received){
			if (sceKernelGetSystemTimeWide() - begin < 10000000){
				int received = 0;
				for(int i = 0;i < sizeof(_gamemode_replicas) / sizeof(_gamemode_replicas[0]);i++){
					if (_gamemode_replicas[i] != NULL && _gamemode_replicas[i]->data_updated){
						received++;
					}
				}
				if (received < _gamemode_num_peers){
					printk("%s: waiting for other masters, %d out of %d ready\n", __func__, received, _gamemode_num_peers);
					sceKernelUnlockLwMutex(&_gamemode_lock, 1);
					sceKernelDelayThread(GAMEMODE_UPDATE_INTERVAL_USEC);
					_gamemode.data_updated = 1;
					continue;
				}
				printk("%s: all %d out of %d masters ready\n", __func__, received, _gamemode_num_peers);
				_gamemode_all_received = 1;
			}else{
				printk("%s: progressing without waiting for other masters\n", __func__);
				_gamemode_all_received = 1;
			}
		}

		sceKernelUnlockLwMutex(&_gamemode_lock, 1);
		sceKernelDelayThread(GAMEMODE_UPDATE_INTERVAL_USEC);
	}

	printk("%s: gamemode master thread stopped\n", __func__);
	_gamemode_stop_thread = -1;
	return 0;
}

/**
 * Adhoc Emulator Gamemode Master Buffer Creator
 * @param ptr Buffer Address
 * @param size Buffer Size (in Bytes)
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_DATALEN, ADHOC_NOT_IN_GAMEMODE, ADHOC_ALREADY_CREATED, NET_NO_SPACE
 */
int proNetAdhocGameModeCreateMaster(const void * ptr, uint32_t size)
{
	sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
		printk("%s: 0x%x %u, 0x%x\n", __func__, ptr, size, _v); \
		return _v; \
	}

	if (!_init)
	{
		RETURN_UNLOCK(ADHOC_NOT_INITIALIZED);
	}

	// Check if we're in game mode
	SceNetAdhocctlGameModeInfo gamemode_info;
	int gamemode_info_get_status = sceNetAdhocctlGetGameModeInfo(&gamemode_info);
	if (gamemode_info_get_status != 0)
	{
		RETURN_UNLOCK(ADHOC_NOT_IN_GAMEMODE);
	}

	if (size < 0 || ptr == NULL)
	{
		RETURN_UNLOCK(ADHOC_INVALID_ARG);
	}

	if (_gamemode.data != NULL)
	{
		RETURN_UNLOCK(ADHOC_ALREADY_CREATED);
	}

	memset(&_gamemode, 0, sizeof(GamemodeInternal));
	sceNetGetLocalEtherAddr(&(_gamemode.saddr));
	_gamemode.data = (void *)ptr;
	_gamemode.data_size = size;

	_gamemode.recv_buf = malloc(_gamemode.data_size);
	if (_gamemode.recv_buf == NULL)
	{
		printk("%s: failed creating data buffer\n", __func__);
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	if (_gamemode_socket_users == 0)
	{
		// XXX the size here would be wrong for shared socket, we're just riding the fact that inet sockets don't have a recv buffer size setting
		_gamemode_socket = sceNetAdhocPdpCreate(&(_gamemode.saddr), ADHOC_GAMEMODE_PORT, size, 0);

		if (_gamemode_socket < 0)
		{
			printk("%s: failed creating gamemode socket, 0x%x\n", __func__, _gamemode_socket);
			free(_gamemode.recv_buf);
			memset(&_gamemode, 0, sizeof(GamemodeInternal));
			RETURN_UNLOCK(NET_NO_SPACE);
		}
	}

	_gamemode_thread_id = sceKernelCreateThread("master data send thread", gamemode_master_thread, 111, 1024 * 8, 0, NULL);
	if (_gamemode_thread_id < 0)
	{
		printk("%s: failed creating broadcast thread, 0x%x\n", __func__, _gamemode_thread_id);
		if (_gamemode_socket_users == 0)
		{
			sceNetAdhocPdpDelete(_gamemode_socket, 0);
			_gamemode_socket = -1;
		}
		free(_gamemode.recv_buf);
		memset(&_gamemode, 0, sizeof(GamemodeInternal));
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	// Going by PPSSPP, the initial data has to be sent
	memcpy(_gamemode.recv_buf, _gamemode.data, _gamemode.data_size);
	_gamemode.data_updated = 1;

	// Send once
	//sceNetAdhocPdpSend(_gamemode_socket, &_broadcast_mac, ADHOC_GAMEMODE_PORT, _gamemode.recv_buf, _gamemode.data_size, 0, 1);

	_gamemode_stop_thread = 0;
	_gamemode_num_peers = gamemode_info.num - 1;
	_gamemode_all_received = 0;
	int thread_start_status = sceKernelStartThread(_gamemode_thread_id, 0, NULL);
	if (thread_start_status < 0)
	{
		// can it fail?
		printk("%s: failed starting broadcast thread, 0x%x\n", __func__, thread_start_status);
		int thread_delete_status = sceKernelDeleteThread(_gamemode_thread_id);
		if (thread_delete_status < 0)
		{
			// uhhhh
			printk("%s: failed removing broadcast thread, 0x%x\n", __func__, thread_delete_status);
		}
		_gamemode_thread_id = -1;

		if (_gamemode_socket_users == 0)
		{
			sceNetAdhocPdpDelete(_gamemode_socket, 0);
			_gamemode_socket = -1;
		}
		free(_gamemode.recv_buf);
		memset(&_gamemode, 0, sizeof(GamemodeInternal));
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	// Master created
	_gamemode_socket_users++;

	RETURN_UNLOCK(0);
}

