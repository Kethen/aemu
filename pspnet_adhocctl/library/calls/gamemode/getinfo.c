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
 * Returns GameMode Specific Information
 * @param info OUT: GameMode Information
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOCCTL_NOT_ENTER_GAMEMODE
 */
int proNetAdhocctlGetGameModeInfo(SceNetAdhocctlGameModeInfo * info)
{
	if (!_init)
	{
		return ADHOCCTL_NOT_INITIALIZED;
	}

	if (info == NULL)
	{
		return ADHOCCTL_INVALID_ARG;
	}

	if (!_in_gamemode)
	{
		return ADHOCCTL_NOT_ENTER_GAMEMODE;
	}

	SceNetAdhocctlPeerInfo peer_info[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
	int buf_len = ADHOCCTL_GAMEMODE_MAX_MEMBERS * sizeof(SceNetAdhocctlPeerInfo);
	int peer_list_get_status = proNetAdhocctlGetPeerList(&buf_len, peer_info);

	SceNetEtherAddr local_mac = {0};
	sceNetGetLocalEtherAddr(&local_mac);

	// Host in front, self next, then the rest
	int is_host = _isMacMatch(&_gamemode_host, &local_mac);
	info->num = is_host ? 1 : 2;
	info->member[0] = _gamemode_host;
	if (!is_host)
	{
		info->member[1] = local_mac;
	}

	if (peer_list_get_status == 0)
	{
		int num_entries = buf_len / sizeof(SceNetAdhocctlPeerInfo);
		for (int i = 0;i < num_entries;i++)
		{
			if (!_isMacMatch(&_gamemode_host, &(peer_info[i].mac_addr)) && !_isMacMatch(&local_mac, &(peer_info[i].mac_addr)))
			{
				info->member[info->num] = peer_info[i].mac_addr;

				(info->num)++;
				if (info->num == ADHOCCTL_GAMEMODE_MAX_MEMBERS)
				{
					break;
				}
			}
		}
	}

	return 0;
}
