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
 * Acquire Peer Linked List
 * @param buflen IN: Length of Buffer in Bytes OUT: Required Length of Buffer in Bytes
 * @param buf Peer List Buffer (can be NULL if you wish to receive Required Length)
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG
 */
int proNetAdhocctlGetPeerList(int * buflen, SceNetAdhocctlPeerInfo * buf)
{
	// Initialized Library
	if(_init == 1)
	{
		// Minimum Arguments
		if(buflen != NULL)
		{
			// Multithreading Lock
			_acquirePeerLock();
			
			// Length Calculation Mode
			if(buf == NULL) *buflen = _getActivePeerCount() * sizeof(SceNetAdhocctlPeerInfo);
			
			// Normal Mode
			else
			{
				// Discovery Counter
				int discovered = 0;
				
				// Calculate Request Count
				int requestcount = *buflen / sizeof(SceNetAdhocctlPeerInfo);
				
				// Clear Memory
				memset(buf, 0, *buflen);
				
				// Minimum Arguments
				if(requestcount > 0)
				{
					#ifdef LOCALHOST_AS_PEER
					// Get Local IP Address
					union SceNetApctlInfo info; if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0)
					{
						// Add Local Address
						buf[discovered].nickname = _parameter.nickname;
						sceWlanGetEtherAddr((void *)buf[discovered].mac_addr.data);
						sceNetInetInetAton(info.ip, &buf[discovered].ip_addr);
						buf[discovered++].last_recv = sceKernelGetSystemTimeWide();
					}
					#endif
					
					// Peer Reference
					SceNetAdhocctlPeerInfoEmu * peer = _friends;
					
					// Iterate Peers
					for(; peer != NULL && discovered < requestcount; peer = peer->next)
					{
						// Fake Receive Time
						peer->last_recv = sceKernelGetSystemTimeWide();
						
						// Copy Peer Info
						buf[discovered].nickname = peer->nickname;
						// PPSSPP sets this
						buf[discovered].nickname.data[ADHOCCTL_NICKNAME_LEN - 1] = 0;
						buf[discovered].mac_addr = peer->mac_addr;
						buf[discovered].padding = 0;
						// PPSSPP sets this
						buf[discovered].flags = 0x0400;
						buf[discovered].last_recv = peer->last_recv;
						discovered++;
					}
					
					// Link List
					int i = 0; for(; i < discovered - 1; i++)
					{
						// Link Network
						buf[i].next = &buf[i + 1];
					}
					
					// Fix Last Element
					if(discovered > 0) buf[discovered - 1].next = NULL;
				}
				
				// Fix Size
				*buflen = discovered * sizeof(SceNetAdhocctlPeerInfo);
			}
			
			// Multithreading Unlock
			_freePeerLock();
			
			// Return Success
			return 0;
		}
		
		// Invalid Arguments
		return ADHOCCTL_INVALID_ARG;
	}
	
	// Uninitialized Library
	return ADHOCCTL_NOT_INITIALIZED;
}

/**
 * Return Number of active Peers in the same Network as the Local Player
 * @return Number of active Peers
 */
int _getActivePeerCount(void)
{
	// Counter
	int count = 0;
	
	#ifdef LOCALHOST_AS_PEER
	// Increase for Localhost
	count++;
	#endif
	
	// Peer Reference
	SceNetAdhocctlPeerInfoEmu * peer = _friends;
	
	// Iterate Peers
	for(; peer != NULL; peer = peer->next)
	{
		// Increase Counter
		count++;
	}
	
	// Return Result
	return count;
}
