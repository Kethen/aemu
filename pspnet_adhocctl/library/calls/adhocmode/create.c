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

// Function Prototypes
int _validNetworkName(const SceNetAdhocctlGroupName * group_name);

/**
 * Create and / or Join a Virtual Network of the specified Name
 * @param group_name Virtual Network Name
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOCCTL_BUSY
 */
int proNetAdhocctlCreate(const SceNetAdhocctlGroupName * group_name)
{
	// Library initialized
	if(_init == 1)
	{
		// Valid Argument
		if(_validNetworkName(group_name))
		{
			// Disconnected State
			if(_thread_status == ADHOCCTL_STATE_DISCONNECTED)
			{
				// Set Network Name
				if(group_name != NULL) _parameter.group_name = *group_name;
				
				// Reset Network Name
				else memset(&_parameter.group_name, 0, sizeof(_parameter.group_name));
				
				// Prepare Connect Packet
				SceNetAdhocctlConnectPacketC2S packet;
				
				// Clear Packet Memory
				memset(&packet, 0, sizeof(packet));
				
				// Set Packet Opcode
				packet.base.opcode = OPCODE_CONNECT;
				
				// Set Target Group
				if(group_name != NULL) packet.group = *group_name;
				
				// Acquire Network Lock
				_acquireNetworkLock();
				
				// Send Packet
				sceNetInetSend(_metasocket, &packet, sizeof(packet), INET_MSG_DONTWAIT);
				
				// Free Network Lock
				_freeNetworkLock();
				
				// Set HUD Connection Status
				setConnectionStatus(1);
				
				// Return Success
				return 0;
			}
			
			// Connected State
			return ADHOCCTL_BUSY;
		}
		
		// Invalid Argument
		return ADHOCCTL_INVALID_ARG;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}

/**
 * Check whether Network Name contains only valid symbols
 * @param group_name To-be-checked Network Name
 * @return 1 if valid or... 0
 */
int _validNetworkName(const SceNetAdhocctlGroupName * group_name)
{
	// Result
	int valid = 1;
	
	// Name given
	if(group_name != NULL)
	{
		// Iterate Name Characters
		int i = 0; for(; i < ADHOCCTL_GROUPNAME_LEN && valid; i++)
		{
			// End of Name
			if(group_name->data[i] == 0) break;
			
			// Not a digit
			if(group_name->data[i] < '0' || group_name->data[i] > '9')
			{
				// Not 'A' to 'Z'
				if(group_name->data[i] < 'A' || group_name->data[i] > 'Z')
				{
					// Not 'a' to 'z'
					if(group_name->data[i] < 'a' || group_name->data[i] > 'z')
					{
						// Invalid Name
						valid = 0;
					}
				}
			}
		}
	}
	
	// Return Result
	return valid;
}
