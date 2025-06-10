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
void _freeNetworkRecursive(SceNetAdhocctlScanInfo * node);

/**
 * Terminate Adhoc-Control Emulator
 * @return 0 on success or... ADHOCCTL_BUSY
 */
int proNetAdhocctlTerm(void)
{
	// Library initialized
	if(_init == 1)
	{
		// Send shutdown request
		_init = 0;
		
		// Set HUD Connection Status
		setConnectionStatus(0);
		
		// Wait for internal thread cooldown
		while(_init != -1) sceKernelDelayThread(10000);
		
		// Multithreading Lock
		_acquirePeerLock();
		
		// Free Friends Memory
		_freeFriendsRecursive(_friends);
		
		// Delete Friends Reference
		_friends = NULL;
		
		// Multithreading Unlock
		_freePeerLock();

		_acquireGroupLock();

		// Free Group Memory
		_freeNetworkRecursive(_networks);
		
		// Delete Group Reference
		_networks = NULL;

		_freeGroupLock();

		// Delete Socket
		sceNetInetClose(_metasocket);
		
		// Delete Socket Reference
		_metasocket = -1;
		
		// Stop UPNP Library
		int status = 0; sceKernelStopModule(_upnp_uid, 0, NULL, &status, NULL);
		
		// Unload UPNP Library
		sceKernelUnloadModule(_upnp_uid);
		
		// Close Hotspot Connection
		sceNetApctlDisconnect();
		
		// Terminate Access Point Control
		sceNetApctlTerm();

		// clear state
		_in_gamemode = 0;
		_num_gamemode_peers = 0;

		// Issue Library cooldown
		_init = 0;
	}
	
	// Return Success
	return 0;
}

/**
 * Recursive Memory Freeing-Helper for Friend-Structures
 * @param node Current Node in List
 */
void _freeFriendsRecursive(SceNetAdhocctlPeerInfoEmu * node)
{
	// End of List
	if(node == NULL) return;
	
	// Increase Recursion Depth
	_freeFriendsRecursive(node->next);
	
	// Free Memory
	free(node);
}

/**
 * Recursive Memory Freeing-Helper for Group-Structures
 * @param node Current Node in List
 */
void _freeNetworkRecursive(SceNetAdhocctlScanInfo * node)
{
	// End of List
	if(node == NULL) return;
	
	// Increase Recursion Depth
	_freeNetworkRecursive(node->next);
	
	// Free Memory
	free(node);
}
