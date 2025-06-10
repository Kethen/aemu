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

#include "common.h"

/**
 * Resolve IP to MAC
 * @param ip Peer IP Address
 * @param mac OUT: Peer MAC
 * @return 0 on success or... ADHOC_NO_ENTRY
 */
int _resolveIP(uint32_t ip, SceNetEtherAddr * mac)
{
	// Get Local IP Address
	union SceNetApctlInfo info; if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0)
	{
		// Convert IP Address to Numerical Display
		uint32_t _ip = 0; sceNetInetInetAton(info.ip, &_ip);
		
		// Local IP Address Requested
		if(ip == _ip)
		{
			// Return MAC Address
			sceWlanGetEtherAddr((void *)mac->data);
			
			// Return Success
			return 0;
		}
	}
	
	// Multithreading Lock
	_acquirePeerLock();
	
	// Peer Reference
	SceNetAdhocctlPeerInfoEmu * peer = _friends;
	
	// Iterate Peers
	for(; peer != NULL; peer = peer->next)
	{
		// Found Matching Peer
		if(peer->ip_addr == ip)
		{
			// Copy Data
			*mac = peer->mac_addr;
			
			// Multithreading Unlock
			_freePeerLock();
			
			// Return Success
			return 0;
		}
	}
	
	// Multithreading Unlock
	_freePeerLock();
	
	// Peer not found
	return ADHOC_NO_ENTRY;
}

int _isMacMatch(const void *lhs, const void *rhs)
{
	// PPSSPP matches the end 5 bytes, because Gran Turismo modifies the first byte somewhere
	return memcmp(lhs + 1, rhs + 1, 5) == 0;
}

/**
 * Resolve MAC to IP
 * @param mac Peer MAC Address
 * @param ip OUT: Peer IP
 * @return 0 on success or... ADHOC_NO_ENTRY
 */
int _resolveMAC(const SceNetEtherAddr * mac, uint32_t * ip)
{
	// Get Local MAC Address
	uint8_t localmac[6]; sceWlanGetEtherAddr((void *)localmac);
	
	// Local MAC Requested
	if(_isMacMatch(localmac, mac))
	{
		// Get Local IP Address
		union SceNetApctlInfo info; if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0)
		{
			// Return IP Address
			sceNetInetInetAton(info.ip, ip);
			
			// Return Success
			return 0;
		}
	}
	
	// Multithreading Lock
	_acquirePeerLock();
	
	// Peer Reference
	SceNetAdhocctlPeerInfoEmu * peer = _friends;
	
	// Iterate Peers
	for(; peer != NULL; peer = peer->next)
	{
		// Found Matching Peer
		if(_isMacMatch(&peer->mac_addr, mac))
		{
			// Copy Data
			*ip = peer->ip_addr;
			
			// Multithreading Unlock
			_freePeerLock();
			
			// Return Success
			return 0;
		}
	}
	
	// Multithreading Unlock
	_freePeerLock();
	
	// Peer not found
	return ADHOC_NO_ENTRY;
}

/**
 * Get First Peer List Element
 * @return First Internal Peer List Element
 */
SceNetAdhocctlPeerInfoEmu * _getInternalPeerList(void)
{
	// Return First Peer List Element
	return _friends;
}

int search_and_join(const SceNetAdhocctlGroupName *group_name, int timeout_usec){
	// Clear currently known networks
	_acquireGroupLock();
	_freeNetworkRecursive(_networks);
	_networks = NULL;
	_freeGroupLock();

	int begin = sceKernelGetSystemTimeLow();
	int found = 0;

	while (sceKernelGetSystemTimeLow() - begin < timeout_usec)
	{
		// Trigger scanning
		if (_thread_status != ADHOCCTL_STATE_SCANNING)
		{
			proNetAdhocctlScan();
		}

		// Wait for scanning to be done
		while(_thread_status == ADHOCCTL_STATE_SCANNING && sceKernelGetSystemTimeLow() - begin < timeout_usec)
		{
			sceKernelDelayThread(10000);
		}

		_acquireGroupLock();
		SceNetAdhocctlScanInfo *group = _networks;
		found = 0;
		while(group != NULL)
		{
			printk("%s: group 0x%x group name %s, looking for %s\n", __func__, group, &group->group_name, group_name);
			if (memcmp(&group->group_name, group_name, sizeof(SceNetAdhocctlGroupName)) == 0)
			{
				found = 1;
				break;
			}
			group = group->next;
		}
		_freeGroupLock();

		if (found){
			break;
		}
		sceKernelDelayThread(10000);
	}

	// Either way we join the group and hope for the best
	int join_result = proNetAdhocctlCreate(group_name);

	if (found)
	{
		printk("%s: network %s found\n", __func__, group_name);
		// 3 seconds padding
		//sceKernelDelayThread(3000000);
	}
	else
	{
		printk("%s: network %s not found\n", __func__, group_name);
	}

	return join_result;
}
