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
 * Join a existing GameMode Network as Peer
 * @param group_name Virtual Network Name
 * @param gc MAC Address of GameMode Host
 * @param timeout Timeout Value (in Microseconds)
 * @param flag Unused Bitflags
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOCCTL_BUSY
 */
int proNetAdhocctlJoinEnterGameMode(const SceNetAdhocctlGroupName * group_name, const SceNetEtherAddr * gc, uint32_t timeout, int flag)
{
	if (!_init)
	{
		return ADHOCCTL_NOT_INITIALIZED;
	}

	if (group_name == NULL || gc == NULL)
	{
		return ADHOCCTL_INVALID_ARG;
	}

	// PPSSPP seems to not check host mac here

	_in_gamemode = -1;
	_joining_gamemode = 1;
	_num_gamemode_peers = 0;

	// join the adhoc network by group name
	//int join_status = proNetAdhocctlCreate(group_name);
	int join_status = search_and_join(group_name, 10000000);
	if (join_status < 0)
	{
		_in_gamemode = 0;
		return join_status;
	}

	// Wait for host to show up
	#if 0
	int begin = sceKernelGetSystemTimeLow();
	while (sceKernelGetSystemTimeLow() - begin < timeout)
	{
		sceKernelDelayThread(10000);
		uint32_t ip;
		if (_resolveMAC(gc, &ip) == 0)
		{
			break;
		}
	}
	#endif

	// Notify, we go into game mode regardless of if host made it
	#if 0
	for (int i = 0; i < ADHOCCTL_MAX_HANDLER; i++)
	{
		// Active Handler
		if(_event_handler[i] != NULL) _event_handler[i](ADHOCCTL_EVENT_GAMEMODE, 0, _event_args[i]);
	}
	#endif

	_maccpy(&_gamemode_host, gc);
	_maccpy(&_actual_gamemode_peers[0], &_gamemode_host);
	sceNetGetLocalEtherAddr(&(_actual_gamemode_peers[1]));
	_num_actual_gamemode_peers = 2;
	_gamemode_host_arrived = 0;
	_gamemode_self_arrived = 0;

	return 0;
}
