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
int _countAvailableNetworks(void);

/**
 * Acquire Network Scan Result in Linked List
 * @param buflen IN: Length of Buffer in Bytes OUT: Required Length of Buffer in Bytes
 * @param buf Network List Buffer (can be NULL if you wish to receive Required Length)
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED
 */
int proNetAdhocctlGetScanInfo(int * buflen, SceNetAdhocctlScanInfo * buf)
{
	// Library initialized
	if(_init == 1)
	{
		// Minimum Argument Requirements
		if(buflen != NULL)
		{
			// Multithreading Lock
			//_acquirePeerLock();
			_acquireGroupLock();
			
			// Length Returner Mode
			if(buf == NULL) *buflen = _countAvailableNetworks() * sizeof(SceNetAdhocctlScanInfo);
			
			// Normal Information Mode
			else
			{
				// Clear Memory
				memset(buf, 0, *buflen);
				
				// Network Discovery Counter
				int discovered = 0;
				
				// Count requested Networks
				int requestcount = *buflen / sizeof(SceNetAdhocctlScanInfo);
				
				// Minimum Argument Requirements
				if(requestcount > 0)
				{
					// Group List Element
					SceNetAdhocctlScanInfo * group = _networks;
					
					// Iterate Group List
					for(; group != NULL && discovered < requestcount; group = group->next)
					{
						// Copy Group Information
						buf[discovered] = *group;
						
						// Exchange Adhoc Channel
						sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_ADHOC_CHANNEL, &buf[discovered].channel);

						printk("%s: setting channel %d for group %s (0x%x next 0x%x)\n", __func__, buf[discovered].channel, &buf[discovered].group_name, group, group->next);

						// Fake Channel Number 1 on Automatic Channel
						if(buf[discovered].channel == 0) buf[discovered].channel = 1;
						
						// Increase Discovery Counter
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
				*buflen = discovered * sizeof(SceNetAdhocctlScanInfo);
			}
			
			// Multithreading Unlock
			//_freePeerLock();
			_freeGroupLock();
			
			// Return Success
			return 0;
		}
		
		// Generic Error
		return -1;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}

/**
 * Count Virtual Networks by analyzing the Friend List
 * @return Number of Virtual Networks
 */
int _countAvailableNetworks(void)
{
	// Network Count
	int count = 0;
	
	// Group Reference
	SceNetAdhocctlScanInfo * group = _networks;
	
	// Count Groups
	for(; group != NULL; group = group->next) count++;
	
	// Return Network Count
	return count;
}
