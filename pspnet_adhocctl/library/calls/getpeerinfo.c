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

#include "../common.h"

/**
 * Acquire Network Scan Result in Linked List
 * @param addr Peer MAC Address
 * @param size Length of peer_info in Bytes
 * @param peer_info OUT: Peer Information
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOC_NO_ENTRY
 */
int proNetAdhocctlGetPeerInfo(SceNetEtherAddr * addr, int size, SceNetAdhocctlPeerInfo * peer_info)
{
	// Library initialized
	if(_init == 1)
	{
		// Valid Arguments
		if(addr != NULL && size >= sizeof(SceNetAdhocctlPeerInfo) && peer_info != NULL)
		{
			// Clear Memory
			memset(peer_info, 0, size);
			
			// Get Local MAC Address
			uint8_t localmac[6]; sceWlanGetEtherAddr((void *)localmac);
			
			// Local MAC Matches
			if(_isMacMatch(localmac, addr))
			{
				// Get Local IP Address
				union SceNetApctlInfo info; if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0)
				{
					// Add Local Address
					peer_info->nickname = _parameter.nickname;
					// PPSSPP sets this
					peer_info->nickname.data[ADHOCCTL_NICKNAME_LEN - 1] = 0;
					peer_info->mac_addr = _parameter.bssid.mac_addr;
					peer_info->padding = 0;
					// PPSSPP sets this
					peer_info->flags = 0x0400;
					//sceNetInetInetAton(info.ip, &peer_info->ip_addr);
					peer_info->last_recv = sceKernelGetSystemTimeWide();
					peer_info->next = NULL;
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
				if(_isMacMatch(&peer->mac_addr, addr))
				{
					// Fake Receive Time
					peer->last_recv = sceKernelGetSystemTimeWide();

					peer_info->nickname = peer->nickname;
					// PPSSPP sets this
					peer_info->nickname.data[ADHOCCTL_NICKNAME_LEN - 1] = 0;
					peer_info->mac_addr = peer->mac_addr;
					peer_info->padding = 0;
					// PSSPP sets this
					peer_info->flags = 0x0400;
					peer_info->last_recv = peer->last_recv;
					peer_info->next = NULL;
					
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
		
		// Invalid Arguments
		return ADHOCCTL_INVALID_ARG;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}
