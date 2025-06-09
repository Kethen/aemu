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
 * Adhoc Emulator Gamemode Peer Replica Buffer Deleter
 * @param id Replica ID
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_NOT_CREATED
 */
int proNetAdhocGameModeDeleteReplica(int id)
{
	sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
		printk("%s: %d, 0x%x\n", __func__, id, _v); \
		return _v; \
	}

	if (!_init)
	{
		RETURN_UNLOCK(ADHOC_NOT_INITIALIZED);
	}

	if (id <= 0 || id > sizeof(_gamemode_replicas) / sizeof(GamemodeInternal *))
	{
		RETURN_UNLOCK(ADHOC_NOT_CREATED);
	}

	GamemodeInternal *gamemode = _gamemode_replicas[id - 1];
	if (gamemode == NULL)
	{
		RETURN_UNLOCK(ADHOC_NOT_CREATED);
	}

	gamemode->stop_thread = 1;

	// Make sure it's stopped
	sceKernelWaitThreadEnd(gamemode->thread_id, NULL);

	// Delete thread
	int thread_delete_status = sceKernelDeleteThread(gamemode->thread_id);
	if (thread_delete_status < 0)
	{
		printk("%s: failed removing receive thread, 0x%x\n", __func__, thread_delete_status);
	}

	// Remove the pdp socket
	sceNetAdhocPdpDelete(gamemode->pdp_sock_id, 0);

	// Free data
	free(gamemode->recv_buf);
	free(gamemode);
	_gamemode_replicas[id - 1] = NULL;

	RETURN_UNLOCK(0);
}
