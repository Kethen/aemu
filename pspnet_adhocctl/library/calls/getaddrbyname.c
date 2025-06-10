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

// Function Prototypes
int _getNicknameCount(const SceNetAdhocctlNickname * nickname);

/**
 * Resolve Player Name to Linked List of Peers
 * @param nickname To-be-resolved Player Nickname
 * @param buflen IN: Buffer Length in Bytes OUT: Required / Used Buffer Length in Bytes
 * @param buf OUT: Resolved Peer Information (if NULL then buflen gets filled with required length)
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG
 */
int proNetAdhocctlGetAddrByName(const SceNetAdhocctlNickname * nickname, int * buflen, SceNetAdhocctlPeerInfo * buf)
{
	// Library initialized
	if(_init == 1)
	{
		// Valid Arguments
		if(nickname != NULL && buflen != NULL)
		{
			// Multithreading Lock
			_acquirePeerLock();
			
			// Length Calculation Mode
			if(buf == NULL) *buflen = _getNicknameCount(nickname) * sizeof(SceNetAdhocctlPeerInfo);
			
			// Normal Information Mode
			else
			{
				// Clear Memory
				memset(buf, 0, *buflen);
				
				// Discovered Player Count
				int discovered = 0;
				
				// Calculate Requested Elements
				int requestcount = *buflen / sizeof(SceNetAdhocctlPeerInfo);
				
				// Minimum Space available
				if(requestcount > 0)
				{
					// Local Nickname Matches
					if(strcmp((char *)_parameter.nickname.data, (char *)nickname->data) == 0)
					{
						// Get Local IP Address
						union SceNetApctlInfo info; if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0)
						{
							// Add Local Address
							buf[discovered].nickname = _parameter.nickname;
							// PPSSPP does this
							buf[discovered].nickname.data[ADHOCCTL_NICKNAME_LEN - 1] = 0;
							sceWlanGetEtherAddr((void *)buf[discovered].mac_addr.data);
							buf[discovered].padding = 0;
							// PPSSPP sets this flag
							buf[discovered].flags = 0x0400;
							//sceNetInetInetAton(info.ip, &buf[discovered].ip_addr);
							buf[discovered].last_recv = sceKernelGetSystemTimeWide();
							discovered++;
						}
					}
					
					// Peer Reference
					SceNetAdhocctlPeerInfoEmu * peer = _friends;
					
					// Iterate Peers
					for(; peer != NULL && discovered < requestcount; peer = peer->next)
					{
						// Match found
						if(strcmp((char *)peer->nickname.data, (char *)nickname->data) == 0)
						{
							// Fake Receive Time
							peer->last_recv = sceKernelGetSystemTimeWide();
							
							// Copy Peer Info
							buf[discovered].nickname = peer->nickname;
							// PPSSPP does this
							buf[discovered].nickname.data[ADHOCCTL_NICKNAME_LEN - 1] = 0;
							buf[discovered].mac_addr = peer->mac_addr;
							buf[discovered].padding = 0;
							// PPSSPP sets this flag
							buf[discovered].flags = 0x0400;
							buf[discovered].last_recv = peer->last_recv;
							discovered++;
						}
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
				
				// Fix Buffer Size
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
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}

/**
 * Return the Number of Players with the chosen Nickname in the Local Users current Network
 * @param nickname To-be-searched Nickname
 * @return Number of matching Players
 */
int _getNicknameCount(const SceNetAdhocctlNickname * nickname)
{
	// Counter
	int count = 0;
	
	// Local Nickname Matches
	if(strcmp((char *)_parameter.nickname.data, (char *)nickname->data) == 0) count++;
	
	// Peer Reference
	SceNetAdhocctlPeerInfoEmu * peer = _friends;
	
	// Iterate Peers
	for(; peer != NULL; peer = peer->next)
	{
		// Match found
		if(strcmp((char *)peer->nickname.data, (char *)nickname->data) == 0) count++;
	}
	
	// Return Result
	return count;
}
