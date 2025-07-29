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

int _num_gamemode_peers = 0;
SceNetEtherAddr _gamemode_peers[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
int _num_actual_gamemode_peers = 0;
SceNetEtherAddr _actual_gamemode_peers[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
int _joining_gamemode = 0;
int _in_gamemode = 0;
SceNetEtherAddr _gamemode_host;
int _gamemode_host_arrived = 0;
int _gamemode_self_arrived = 0;
int _gamemode_notified = 0;

uint64_t _gamemode_join_timestamp = 0;

/**
 * Create and Join a GameMode Network as Host
 * @param group_name Virtual Network Name
 * @param game_type Network Type (1A, 1B, 2A)
 * @param num Total Number of Peers (including Host)
 * @param members MAC Address List of Peers (own MAC at Index 0)
 * @param timeout Timeout Value (in Microseconds)
 * @param flag Unused Bitflags
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOCCTL_BUSY, ADHOCCTL_CHANNEL_NOT_AVAILABLE
 */
int proNetAdhocctlCreateEnterGameMode(const SceNetAdhocctlGroupName * group_name, int game_type, int num, const SceNetEtherAddr * members, uint32_t timeout, int flag)
{
	if (!_init)
	{
		return ADHOCCTL_NOT_INITIALIZED;
	}

	if (group_name == NULL || num < 0)
	{
		return ADHOCCTL_INVALID_ARG;
	}

	if (num > ADHOCCTL_GAMEMODE_MAX_MEMBERS)
	{
		printk("%s: okay... the game is supplying more than %d macs (%d), not good\n", __func__, ADHOCCTL_GAMEMODE_MAX_MEMBERS, num);
		return ADHOCCTL_INVALID_ARG;
	}

	// XXX this might have to be on another thread? The real impl seems to be non block

	_in_gamemode = -1;
	_joining_gamemode = 0;
	_gamemode_notified = 0;

	// save member list
	memcpy(_gamemode_peers, members, sizeof(SceNetEtherAddr) * num);
	_num_gamemode_peers = num;

	// join the adhoc network by group name
	int join_status = proNetAdhocctlCreate(group_name);
	if (join_status < 0)
	{
		_in_gamemode = 0;
		return join_status;
	}

	sceNetGetLocalEtherAddr(&_gamemode_host);
	_maccpy(&_actual_gamemode_peers[0], &_gamemode_host);
	_num_actual_gamemode_peers = 1;
	_gamemode_host_arrived = 0;
	_gamemode_self_arrived = 0;

	_gamemode_join_timestamp = sceKernelGetSystemTimeWide();

	#if 1
	uint64_t begin = sceKernelGetSystemTimeWide();
	while(!_gamemode_notified && sceKernelGetSystemTimeWide() - begin < 5000000)
	{
		sceKernelDelayThread(100000);
	}
	#endif

	return 0;
}

void _appendGamemodePeer(SceNetEtherAddr *peer)
{
	if (_num_actual_gamemode_peers >= ADHOCCTL_GAMEMODE_MAX_MEMBERS)
	{
		return;
	}
	_maccpy(&_actual_gamemode_peers[_num_actual_gamemode_peers], peer);
	_num_actual_gamemode_peers++;
}

void _insertGamemodePeer(SceNetEtherAddr *peer)
{
	if (_num_actual_gamemode_peers >= ADHOCCTL_GAMEMODE_MAX_MEMBERS)
	{
		return;
	}

	SceNetEtherAddr *begin = _joining_gamemode ? &(_actual_gamemode_peers[2]) : &(_actual_gamemode_peers[1]);
	int num_slots = _num_actual_gamemode_peers - (_joining_gamemode ? 2 : 1);

	// Move all items one slot right
	for (int i = num_slots; i > 0;i--)
	{
		_maccpy(&begin[i], &begin[i - 1]);
	}

	SceNetEtherAddr self;
	sceNetGetLocalEtherAddr(&self);
	printk("%s: self %x:%x:%x:%x:%x:%x\n", __func__, self.data[0], self.data[1], self.data[2], self.data[3], self.data[4], self.data[5]);
	printk("%s: host %x:%x:%x:%x:%x:%x\n", __func__, _gamemode_host.data[0], _gamemode_host.data[1], _gamemode_host.data[2], _gamemode_host.data[3], _gamemode_host.data[4], _gamemode_host.data[5]);
	printk("%s: inserting %x:%x:%x:%x:%x:%x\n", __func__, peer->data[0], peer->data[1], peer->data[2], peer->data[3], peer->data[4], peer->data[5]);

	// Place new peer
	_maccpy(&begin[0], peer);

	// Incrememt peer count
	_num_actual_gamemode_peers++;
}
