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

int ptp_open_postoffice(const SceNetEtherAddr *saddr, uint16_t sport, const SceNetEtherAddr *daddr, uint16_t dport, uint32_t bufsize){
	AdhocSocket *internal = (AdhocSocket *)malloc(sizeof(AdhocSocket));
	if (internal == NULL){
		printk("%s: ran out of heap memory trying to open ptp socket\n", __func__);
		return NET_NO_SPACE;
	}

	internal->is_ptp = true;
	internal->postoffice_handle = NULL;
	internal->ptp.state = PTP_STATE_CLOSED;
	internal->ptp.laddr = *saddr;
	internal->ptp.lport = sport;
	internal->ptp.paddr = *daddr;
	internal->ptp.pport = dport;
	internal->ptp.rcv_sb_cc = bufsize;
	internal->ptp.snd_sb_cc = 0;
	internal->connect_thread = -1;
	internal->ptp_ext.establish_timestamp = sceKernelGetSystemTimeWide();

	sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
	AdhocSocket **slot = NULL;
	int i;
	for(i = 0;i < 255;i++){
		if(_sockets[i] == NULL){
			slot = &_sockets[i];
			break;
		}
	}

	if (slot == NULL){
		sceKernelSignalSema(_socket_mapper_mutex, 1);
		free(internal);
		printk("%s: cannot find free socket mapper slot while opening ptp socket\n", __func__);
		return NET_NO_SPACE;
	}

	*slot = internal;
	sceKernelSignalSema(_socket_mapper_mutex, 1);

	return i + 1;
}

/**
 * Adhoc Emulator PTP Active Socket Creator
 * @param saddr Local MAC (Unused)
 * @param sport Local Binding Port
 * @param daddr Target MAC
 * @param dport Target Port
 * @param bufsize Socket Buffer Size
 * @param rexmt_int Retransmit Interval (in Microseconds)
 * @param rexmt_cnt Retransmit Count
 * @param flag Bitflags (Unused)
 * @return Socket ID > 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_ADDR, ADHOC_INVALID_PORT
 */
int proNetAdhocPtpOpen(const SceNetEtherAddr * saddr, uint16_t sport, const SceNetEtherAddr * daddr, uint16_t dport, uint32_t bufsize, uint32_t rexmt_int, int rexmt_cnt, int flag)
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

		// Valid Addresses
		//if(saddr != NULL && _IsLocalMAC(saddr) && daddr != NULL && !_isBroadcastMAC(daddr))
		// Reject zero mac like with PPSSPP
		if(saddr != NULL && daddr != NULL && !_isBroadcastMAC(daddr) && !_isZeroMac(daddr))
		{
			// dport cannot be 0
			if (dport == 0)
			{
				return ADHOC_INVALID_PORT;
			}

			// check in-use port here like PPSSPP
			if (sport != 0 && _IsPTPPortInUse(sport, 0, daddr, dport))
			{
				return ADHOC_PORT_IN_USE;
			}

			// Random Port required
			if(sport == 0)
			{
				// Find unused Port
				while(sport == 0 || _IsPTPPortInUse(sport, 0, daddr, dport))
				{
					// Generate Port Number
					sport = (uint16_t)_getRandomNumber(65535);
				}
			}
			
			// Valid Ports
			//if(!_IsPTPPortInUse(sport, 0, daddr, dport) && dport != 0)
			{
				// Valid Arguments
				if(bufsize > 0 && rexmt_int > 0 && rexmt_cnt > 0)
				{
					if (_postoffice){
						return ptp_open_postoffice(&local_mac, sport, daddr, dport, bufsize);
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
									internal->ptp.paddr = *daddr;
									internal->ptp.lport = sport;
									internal->ptp.pport = dport;
									
									// Set Buffer Size
									internal->ptp.rcv_sb_cc = bufsize;
									
									// Link PTP Socket
									_sockets[i] = internal;
									
									// Add Port Forward to Router
									sceNetPortOpen("TCP", sport);

									// Save mode for upnp cleanup
									internal->ptp_ext.mode = PTP_MODE_OPEN;

									sceKernelSignalSema(_socket_mapper_mutex, 1);

									// Return PTP Socket Pointer
									return i + 1;
								}
								sceKernelSignalSema(_socket_mapper_mutex, 1);
								
								// Free Memory
								free(internal);
							}
						}
						
						// Close Socket
						sceNetInetClose(socket);
					}
				}
				
				// Invalid Arguments
				return ADHOC_INVALID_ARG;
			}
			
			// Invalid Ports
			//return ADHOC_INVALID_PORT;
		}
		
		// Invalid Addresses
		return ADHOC_INVALID_ADDR;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}

int _isMacMatch(const void *lhs, const void *rhs)
{
	// PPSSPP matches the end 5 bytes, because Gran Turismo modifies the first byte somewhere
	return memcmp(lhs + 1, rhs + 1, 5) == 0;
}

/**
 * Check whether PTP Port is in use or not
 * @param port To-be-checked Port Number
 * @return 1 if in use or... 0
 */
int _IsPTPPortInUse(uint16_t sport, int listen, const SceNetEtherAddr *daddr, uint16_t dport)
{
	// Iterate Sockets
	for (int i = 0; i < 255; i++) {
		if (_sockets[i] != NULL && _sockets[i]->is_ptp)
		{
			// Based on PPSSPP's new implementation
			// Fingers crossed that the PSP has netbsd inet socket, and allows open + listen on the same sport
			SceNetAdhocPtpStat *sock = &_sockets[i]->ptp;
			if (sock->lport == sport &&
			    ((listen && sock->state == PTP_STATE_LISTEN) ||
			     (!listen && sock->state != PTP_STATE_LISTEN &&
			      sock->pport == dport && daddr != NULL && _isMacMatch(&sock->paddr, daddr)))) 
			{
				return 1;
			}
		}
	}
	
	// Unused Port
	return 0;
}
