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
 * Adhoc Emulator PDP Socket Creator
 * @param saddr Local MAC (Unused)
 * @param sport Local Binding Port
 * @param bufsize Socket Buffer Size
 * @param flag Bitflags (Unused)
 * @return Socket ID > 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_SOCKET_ID_NOT_AVAIL, ADHOC_INVALID_ADDR, ADHOC_PORT_NOT_AVAIL, ADHOC_INVALID_PORT, ADHOC_PORT_IN_USE, NET_NO_SPACE
 */
int proNetAdhocPdpCreate(const SceNetEtherAddr * saddr, uint16_t sport, int bufsize, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Arguments are supplied
		if(saddr != NULL && bufsize > 0)
		{
			// Just force local mac for now, seems fine on PPSSPP
			SceNetEtherAddr local_mac = {0};
			sceNetGetLocalEtherAddr(&local_mac);
			#ifdef DEBUG
			if (memcmp(&local_mac, saddr, ETHER_ADDR_LEN) != 0)
			{
				printk("%s: createing pdp with a non local mac..? local %x:%x:%x:%x:%x:%x desired %x:%x:%x:%x:%x:%x\n", __func__, (uint32_t)(local_mac.data[0]), (uint32_t)(local_mac.data[1]), (uint32_t)(local_mac.data[2]), (uint32_t)(local_mac.data[3]), (uint32_t)(local_mac.data[4]), (uint32_t)(local_mac.data[5]), (uint32_t)(saddr->data[0]), (uint32_t)(saddr->data[1]), (uint32_t)(saddr->data[2]), (uint32_t)(saddr->data[3]), (uint32_t)(saddr->data[4]), (uint32_t)(saddr->data[5]));
			}
			#endif
			// Valid MAC supplied
			//if(_IsLocalMAC(saddr))
			{
				// Random Port required
				if(sport == 0)
				{
					// Find unused Port
					while(sport < 0 || _IsPDPPortInUse(sport))
					{
						// Generate Port Number
						sport = (uint16_t)_getRandomNumber(65535);
					}
				}else
				{
					// Warn about priv ports for playing with PPSSPP
					if (__builtin_expect(sport < 1024, 0))
						printk("%s: using sport %d, might have issues when playing with PPSSPP, change your port offset here and on PPSSPP\n", __func__, sport);
				}
				
				// Unused Port supplied
				if(!_IsPDPPortInUse(sport))
				{
					// Create Internet UDP Socket
					int socket = sceNetInetSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
					
					// Valid Socket produced
					if(socket > 0)
					{
						// Enable Port Re-use
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &_one, sizeof(_one));
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &_one, sizeof(_one));
						
						// Binding Information for local Port
						SceNetInetSockaddrIn addr;
						addr.sin_len = sizeof(addr);
						addr.sin_family = AF_INET;
						addr.sin_addr = INADDR_ANY;
						addr.sin_port = sceNetHtons(sport);
						
						// Bound Socket to local Port
						if(sceNetInetBind(socket, (SceNetInetSockaddr *)&addr, sizeof(addr)) == 0)
						{
							// Allocate Memory for Internal Data
							SceNetAdhocPdpStat * internal = (SceNetAdhocPdpStat *)malloc(sizeof(SceNetAdhocPdpStat));
							
							// Allocated Memory
							if(internal != NULL)
							{
								// Clear Memory
								memset(internal, 0, sizeof(SceNetAdhocPdpStat));
								
								// Find Free Translator Index
								int i = 0; for(; i < 255; i++) if(_pdp[i] == NULL) break;
								
								// Found Free Translator Index
								if(i < 255)
								{
									// Fill in Data
									internal->id = socket;
									//internal->laddr = *saddr;
									internal->laddr = local_mac;
									internal->lport = sport;
									internal->rcv_sb_cc = bufsize;
									
									// Link Socket to Translator ID
									_pdp[i] = internal;
									
									// Forward Port on Router
									sceNetPortOpen("UDP", sport);
									
									// Success
									return i + 1;
								}
								
								// Free Memory for Internal Data
								free(internal);
							}
						}
						
						// Close Socket
						sceNetInetClose(socket);
					}
					
					// Default to No-Space Error
					return NET_NO_SPACE;
				}
				
				// Port is in use by another PDP Socket
				return ADHOC_PORT_IN_USE;
			}
			
			// Invalid MAC supplied
			return ADHOC_INVALID_ADDR;
		}
		
		// Invalid Arguments were supplied
		return ADHOC_INVALID_ARG;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}

/**
 * Local MAC Check
 * @param saddr To-be-checked MAC Address
 * @return 1 if valid or... 0
 */
int _IsLocalMAC(const SceNetEtherAddr * addr)
{
	// Get Local MAC Address
	SceNetEtherAddr saddr;
	sceNetGetLocalEtherAddr(&saddr);
	
	// Compare MAC Addresses
	int match = memcmp((const void *)addr, (const void *)&saddr, ETHER_ADDR_LEN);
	
	// Return Result
	return (match == 0);
}

/**
 * PDP Port Check
 * @param port To-be-checked Port
 * @return 1 if in use or... 0
 */
int _IsPDPPortInUse(uint16_t port)
{
	// Iterate Elements
	int i = 0; for(; i < 255; i++) if(_pdp[i] != NULL && _pdp[i]->lport == port) return 1;
	
	// Unused Port
	return 0;
}
