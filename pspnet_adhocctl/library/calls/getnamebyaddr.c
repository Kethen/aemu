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
 * Resolve Player Name to Linked List of Peers
 * @param addr To-be-resolved MAC Address
 * @param nickname OUT: Resolved Nickname
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOC_NO_ENTRY
 */
int proNetAdhocctlGetNameByAddr(const SceNetEtherAddr * addr, SceNetAdhocctlNickname * nickname)
{
	// Library initialized
	if(_init == 1)
	{
		// Valid Arguments
		if(addr != NULL && nickname != NULL)
		{
			// Get Local MAC Address
			uint8_t localmac[6]; sceWlanGetEtherAddr((void *)localmac);
			
			// Local MAC Matches
			if(_isMacMatch(localmac, addr))
			{
				// Write Data
				*nickname = _parameter.nickname;
				
				// Return Success
				return 0;
			}
			
			// Multithreading Lock
			_acquirePeerLock();
			
			// Peer Reference
			SceNetAdhocctlPeerInfoEmu * peer = _friends;
			
			// Iterate Peers
			for(; peer != NULL; peer = peer->next)
			{
				// Match found
				if(_isMacMatch(&peer->mac_addr, addr))
				{
					// Write Data
					*nickname = peer->nickname;
					
					// Multithreading Unlock
					_freePeerLock();
					
					// Return Success
					return 0;
				}
			}
			
			// Multithreading Unlock
			_freePeerLock();
			
			// Player not found
			return ADHOC_NO_ENTRY;
		}
		
		// Invalid Arguments
		return ADHOCCTL_INVALID_ARG;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}
