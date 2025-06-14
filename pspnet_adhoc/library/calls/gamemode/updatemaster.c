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
 * Adhoc Emulator Gamemode Master Buffer Updater
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_DATALEN, ADHOC_NOT_IN_GAMEMODE, ADHOC_ALREADY_CREATED, NET_NO_SPACE
 */
int proNetAdhocGameModeUpdateMaster(void)
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

	// Check if there is a master
	if (_gamemode.data == NULL)
	{
		RETURN_UNLOCK(ADHOC_NOT_CREATED);
	}

	// Copy data to data buffer and send once 
	memcpy(_gamemode.recv_buf, _gamemode.data, _gamemode.data_size);
	// Force send once
	sceNetAdhocPdpSend(_gamemode_socket, &_broadcast_mac, ADHOC_GAMEMODE_PORT, _gamemode.recv_buf, _gamemode.data_size, 0, 1);

	// Send data
	_gamemode.data_updated = 1;

	RETURN_UNLOCK(0);
}
