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


GamemodeInternal _gamemode = {0};
SceLwMutexWorkarea _gamemode_lock = {0};
const SceNetEtherAddr _broadcast_mac = {.data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

static int gamemode_master_thread(SceSize args, void *argp)
{
	sceKernelDelayThread(GAMEMODE_INIT_DELAY_USEC);
	while(!_gamemode.stop_thread)
	{
		// yolo braodcast
		// in the PPSSPP implementation, adhoc ctl should update gamemode peer status, as more peers are inserted
		// here we instead send on update and keep a background ping, let's see how it goes...

		sceNetAdhocPdpSend(_gamemode.pdp_sock_id, &_broadcast_mac, ADHOC_GAMEMODE_PORT, _gamemode.data, _gamemode.data_size, 0, 0);

		sceKernelDelayThread(1000000);
	}
	_gamemode.stop_thread = -1;
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
		printk("%s: 0x%x %u, 0x%x", __func__, ptr, size, _v); \
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
	_gamemode.data_updated = 1;
	_gamemode.pdp_sock_id = sceNetAdhocPdpCreate(&(_gamemode.saddr), ADHOC_GAMEMODE_PORT, size, 0);

	if (_gamemode.pdp_sock_id < 0)
	{
		printk("%s: failed creating pdp send socket, 0x%x\n", __func__, _gamemode.pdp_sock_id);
		memset(&_gamemode, 0, sizeof(GamemodeInternal));
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	_gamemode.thread_id = sceKernelCreateThread("master data send thread", gamemode_master_thread, 111, 1024 * 4, 0, NULL);
	if (_gamemode.thread_id < 0)
	{
		printk("%s: failed creating broadcast thread, 0x%x\n", __func__, _gamemode.thread_id);
		sceNetAdhocPdpDelete(_gamemode.pdp_sock_id, 0);
		memset(&_gamemode, 0, sizeof(GamemodeInternal));
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	int thread_start_status = sceKernelStartThread(_gamemode.thread_id, 0, NULL);
	if (thread_start_status < 0)
	{
		// can it fail?
		printk("%s: failed starting broadcast thread, 0x%x\n", __func__, thread_start_status);
		int thread_delete_status = sceKernelDeleteThread(_gamemode.thread_id);
		if (thread_delete_status < 0)
		{
			// uhhhh
			printk("%s: failed removing broadcast thread, 0x%x\n", __func__, thread_delete_status);
		}
		sceNetAdhocPdpDelete(_gamemode.pdp_sock_id, 0);
		memset(&_gamemode, 0, sizeof(GamemodeInternal));
		RETURN_UNLOCK(NET_NO_SPACE);
	}

	// master created
	RETURN_UNLOCK(0);
}
