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
 * Adhoc Emulator Gamemode Peer Replica Buffer Updater
 * @param id Replica ID
 * @param opt OUT: Optional Replica Information (NULL can be passed)
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_NOT_IN_GAMEMODE, ADHOC_ALREADY_CREATED, WLAN_INVALID_ARG
 */
int proNetAdhocGameModeUpdateReplica(int id, SceNetAdhocGameModeOptData * opt)
{
	sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&_gamemode_lock, 1); \
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

	if (id > sizeof(_gamemode_replicas) / sizeof(GamemodeInternal *) || id <= 0)
	{
		RETURN_UNLOCK(ADHOC_NOT_CREATED);
	}

	GamemodeInternal *gamemode = _gamemode_replicas[id - 1];

	if (gamemode == NULL)
	{
		RETURN_UNLOCK(ADHOC_NOT_CREATED);
	}

	if (gamemode->data_updated)
	{
		// We have new data
		gamemode->data_updated = 0;
		memcpy(gamemode->data, gamemode->recv_buf, gamemode->data_size);
		if (opt != NULL)
		{
			opt->size = sizeof(SceNetAdhocGameModeOptData);
			opt->flag = 1;
			opt->last_recv = gamemode->last_recv;
		}
		RETURN_UNLOCK(0);
	}

	// no data
	if (opt != NULL)
	{
		opt->size = sizeof(SceNetAdhocGameModeOptData);
		opt->flag = 0;
	}
	RETURN_UNLOCK(0);
}
