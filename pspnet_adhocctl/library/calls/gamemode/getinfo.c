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

	// Does this work before the gamemode event?
	//if (_in_gamemode != 1)
	if (!_in_gamemode)
	{
		return ADHOCCTL_NOT_ENTER_GAMEMODE;
	}

	info->num = _num_actual_gamemode_peers;
	for (int i = 0; i < info->num;i++)
	{
		_maccpy(&(info->member[i]), &(_actual_gamemode_peers[i]));
	}

	return 0;
}
