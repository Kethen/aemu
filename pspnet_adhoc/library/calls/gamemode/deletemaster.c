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

/**
 * Adhoc Emulator Gamemode Master Buffer Deleter
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_NOT_CREATED
 */
int proNetAdhocGameModeDeleteMaster(void)
{
	sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
		printk("%s: 0x%x", __func__, _v); \
		return _v; \
	}

	if (!_init)
	{
		RETURN_UNLOCK(ADHOC_NOT_INITIALIZED);
	}

	// Delete master if we have a master
	#if 0
	// Check if we're in game mode
	SceNetAdhocctlGameModeInfo gamemode_info;
	int gamemode_info_get_status = sceNetAdhocctlGetGameModeInfo(&gamemode_info);
	if (gamemode_info_get_status != 0)
	{
		RETURN_UNLOCK(ADHOC_NOT_IN_GAMEMODE);
	}
	#endif

	// Check if there is a master
	if (_gamemode.data == NULL)
	{
		RETURN_UNLOCK(ADHOC_NOT_CREATED);
	}

	// Remove the master thread
	_gamemode.stop_thread = 1;

	// Making sure it has stopped
	sceKernelWaitThreadEnd(_gamemode.thread_id, NULL);

	// Delete Thread
	int thread_delete_status = sceKernelDeleteThread(_gamemode.thread_id);
	if (thread_delete_status < 0)
	{
		printk("%s: failed removing broadcast thread, 0x%x\n", __func__, thread_delete_status);
	}

	// Remove the pdp socket
	sceNetAdhocPdpDelete(_gamemode.pdp_sock_id, 0);

	// Clean data
	free(_gamemode.recv_buf);
	memset(&_gamemode, 0, sizeof(GamemodeInternal));

	RETURN_UNLOCK(0);
}
