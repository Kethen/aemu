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

static int ptp_listen_postoffice(const SceNetEtherAddr *saddr, uint16_t sport, uint32_t bufsize){
	// server connect delegated to accept
	AdhocSocket *internal = (AdhocSocket *)malloc(sizeof(AdhocSocket));
	if (internal == NULL){
		printk("%s: out of heap memory when creating ptp listen socket\n", __func__);
		return NET_NO_SPACE;
	}

	internal->is_ptp = true;
	internal->postoffice_handle = NULL;
	internal->ptp.laddr = *saddr;
	internal->ptp.lport = sport;
	internal->ptp.state = PTP_STATE_LISTEN;
	internal->ptp.rcv_sb_cc = bufsize;
	internal->ptp.snd_sb_cc = 0;
	internal->connect_thread = -1;

	sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
	AdhocSocket **slot = NULL;
	int i;
	for(i = 0;i < 255;i++){
		if (_sockets[i] == NULL){
			slot = &_sockets[i];
			break;
		}
	}

	if (slot == NULL){
		sceKernelSignalSema(_socket_mapper_mutex, 1);
		printk("%s: out of socket slots when creating adhoc ptp listen socket\n", __func__);
		free(internal);
		return NET_NO_SPACE;
	}

	*slot = internal;
	sceKernelSignalSema(_socket_mapper_mutex, 1);
	
	return i + 1;
}

/**
 * Adhoc Emulator PTP Passive Socket Creator
 * @param saddr Local MAC (Unused)
 * @param sport Local Binding Port
 * @param bufsize Socket Buffer Size
 * @param rexmt_int Retransmit Interval (in Microseconds)
 * @param rexmt_cnt Retransmit Count
 * @param backlog Size of Connection Queue
 * @param flag Bitflags (Unused)
 * @return Socket ID > 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_ADDR, ADHOC_INVALID_PORT, ADHOC_SOCKET_ID_NOT_AVAIL, ADHOC_PORT_NOT_AVAIL, ADHOC_PORT_IN_USE, NET_NO_SPACE
 */
int proNetAdhocPtpListen(const SceNetEtherAddr * saddr, uint16_t sport, uint32_t bufsize, uint32_t rexmt_int, int rexmt_cnt, int backlog, int flag)
{
	// Library is initialized
	if(_init)
	{
		// force local MAC like PPSSPP
		SceNetEtherAddr local_mac = {0};
		sceNetGetLocalEtherAddr(&local_mac);
		#ifdef DEBUG
		if (saddr != NULL && memcmp(&local_mac, saddr, ETHER_ADDR_LEN) != 0)
		{
			printk("%s: createing pdp with a non local mac..? local %x:%x:%x:%x:%x:%x desired %x:%x:%x:%x:%x:%x\n", __func__, (uint32_t)(local_mac.data[0]), (uint32_t)(local_mac.data[1]), (uint32_t)(local_mac.data[2]), (uint32_t)(local_mac.data[3]), (uint32_t)(local_mac.data[4]), (uint32_t)(local_mac.data[5]), (uint32_t)(saddr->data[0]), (uint32_t)(saddr->data[1]), (uint32_t)(saddr->data[2]), (uint32_t)(saddr->data[3]), (uint32_t)(saddr->data[4]), (uint32_t)(saddr->data[5]));
		}
		#endif

		// Valid Address
		//if(saddr != NULL && _IsLocalMAC(saddr))
		if(saddr != NULL)
		{
			// check for in-use port here like PPSSPP
			if (sport != 0 && _IsPTPPortInUse(sport, 1, NULL, 0))
			{
				return ADHOC_PORT_IN_USE;
			}

			// Random Port required
			if(sport == 0)
			{
				// Find unused Port
				while(sport == 0 || _IsPTPPortInUse(sport, 1, NULL, 0))
				{
					// Generate Port Number
					sport = (uint16_t)_getRandomNumber(65535);
				}
			}
			
			// Valid Ports
			//if(!_IsPTPPortInUse(sport, 1, NULL, 0))
			{
				// Valid Arguments
				if(bufsize > 0 && rexmt_int > 0 && rexmt_cnt > 0 && backlog > 0)
				{
					if (_postoffice){
						return ptp_listen_postoffice(&local_mac, sport, bufsize);
					}

					// Create Infrastructure Socket
					int socket = sceNetInetSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					
					// Valid Socket produced
					if(socket > 0)
					{
						// Enable Port Re-use
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &_one, sizeof(_one));
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &_one, sizeof(_one));

						// Enable keep alive like PPSSPP
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &_one, sizeof(_one));

						// Send faster in case of timeout
						//sceNetInetSetsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &_one, sizeof(_one));

						// Binding Information for local Port
						SceNetInetSockaddrIn addr;
						addr.sin_len = sizeof(addr);
						addr.sin_family = AF_INET;
						addr.sin_addr = INADDR_ANY;
						addr.sin_port = sceNetHtons(sport);
						
						// Bound Socket to local Port
						if(sceNetInetBind(socket, (SceNetInetSockaddr *)&addr, sizeof(addr)) == 0)
						{
							// Switch into Listening Mode
							if(sceNetInetListen(socket, backlog) == 0)
							{
								// Allocate Memory
								AdhocSocket *internal = (AdhocSocket *)malloc(sizeof(AdhocSocket));
								
								// Allocated Memory
								if(internal != NULL)
								{
									// Find Free Translator ID
									sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
									int i = 0; for(; i < 255; i++) if(_sockets[i] == NULL) break;
									
									// Found Free Translator ID
									if(i < 255)
									{
										// Clear Memory
										memset(internal, 0, sizeof(AdhocSocket));

										// Tag ptp
										internal->is_ptp = true;
										
										// Copy Infrastructure Socket ID
										internal->ptp.id = socket;
										
										// Copy Address Information
										//internal->ptp.laddr = *saddr;
										internal->ptp.laddr = local_mac;
										internal->ptp.lport = sport;
										
										// Flag Socket as Listener
										internal->ptp.state = PTP_STATE_LISTEN;
										
										// Set Buffer Size
										internal->ptp.rcv_sb_cc = bufsize;
										
										// Link PTP Socket
										_sockets[i] = internal;
										
										// Add Port Forward to Router
										sceNetPortOpen("TCP", sport);

										// Save ptp mode for cleanup
										internal->ptp_ext.mode = PTP_MODE_LISTEN;

										sceKernelSignalSema(_socket_mapper_mutex, 1);

										// Return PTP Socket Pointer
										return i + 1;
									}
									sceKernelSignalSema(_socket_mapper_mutex, 1);
									
									// Free Memory
									free(internal);
								}
							}
						}
						
						// Close Socket
						sceNetInetClose(socket);
					}
					
					// Socket not available
					return ADHOC_SOCKET_ID_NOT_AVAIL;
				}
				
				// Invalid Arguments
				return ADHOC_INVALID_ARG;
			}
			
			// Invalid Ports
			//return ADHOC_PORT_IN_USE;
		}
		
		// Invalid Addresses
		return ADHOC_INVALID_ADDR;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
