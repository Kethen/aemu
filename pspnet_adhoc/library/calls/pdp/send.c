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

#define NBIO_BCAST 1

static int pdp_send_postoffice_unicast(int idx, const SceNetEtherAddr *daddr, uint16_t dport, const void *data, int len, uint32_t timeout, int nonblock){
	uint64_t begin = sceKernelGetSystemTimeWide();
	uint64_t end = begin + timeout;

	if (len > 2048){
		// okay what's with the giant buffers in games
		len = 2048;
	}

	int pdp_send_status = 0;
	int recovery_cnt = 0;
	while(1){
		void *pdp_sock = pdp_postoffice_recover(idx);
		if (pdp_sock == NULL){
			if (_sockets[idx] == NULL){
				// we don't even have a socket anymore, because the game decided to close it on another thread
				return ADHOC_INVALID_SOCKET_ID;
			}
			if (!nonblock && timeout != 0 && sceKernelGetSystemTimeWide() < end){
				sceKernelDelayThread(100);
				continue;
			}
			if (!nonblock && timeout == 0){
				recovery_cnt++;
				if (recovery_cnt == 100){
					printk("%s: server might be down, and game wants to perform blocking send, we're stuck until server comes back\n", __func__);
				}
				sceKernelDelayThread(100);
				continue;
			}
			// we can do recovery later
			pdp_send_status = AEMU_POSTOFFICE_CLIENT_SESSION_WOULD_BLOCK;
			break;
		}
		pdp_send_status = pdp_send(pdp_sock, (const char *)daddr, dport, (char *)data, len, nonblock || timeout != 0);
		if (pdp_send_status == AEMU_POSTOFFICE_CLIENT_SESSION_DEAD){
			// let recovery deal with this
			pdp_delete(pdp_sock);
			_sockets[idx]->postoffice_handle = NULL;
			sceKernelDelayThread(100);
			continue;
		}
		if (pdp_send_status == AEMU_POSTOFFICE_CLIENT_SESSION_WOULD_BLOCK && !nonblock && timeout != 0 && sceKernelGetSystemTimeWide() < end){
			sceKernelDelayThread(100);
			continue;
		}
		break;
	}

	if (pdp_send_status == AEMU_POSTOFFICE_CLIENT_SESSION_WOULD_BLOCK){
		if (nonblock){
			return ADHOC_WOULD_BLOCK;
		}else{
			return ADHOC_TIMEOUT;
		}
	}
	if (pdp_send_status == AEMU_POSTOFFICE_CLIENT_OUT_OF_MEMORY){
		// this is critical
		printk("%s: critical: huge client buf %d what is going on please fix\n", __func__, len);
	}

	return 0;
}

static int pdp_send_postoffice(int idx, const SceNetEtherAddr *daddr, uint16_t dport, const void *data, int len, uint32_t timeout, int nonblock){
	if(!_isBroadcastMAC(daddr)){
		return pdp_send_postoffice_unicast(idx, daddr, dport, data, len, timeout, nonblock);
	}

	// Iterate Peers
	_acquirePeerLock();
	if(!_is_ppsspp){
		for(SceNetAdhocctlPeerInfoEmu * peer = _getInternalPeerList(); peer != NULL; peer = peer->next){
			pdp_send_postoffice_unicast(idx, &peer->mac_addr, dport, data, len, 0, 1);
		}
	}else{
		SceNetAdhocctlPeerInfo peers[32];
		int num_peers = sizeof(peers);
		int fetch_result = sceNetAdhocctlGetPeerList(&num_peers, peers);
		if (fetch_result != 0){
			printk("%s: failed fetching peer list while trying to broadcast, 0x%x\n", __func__, fetch_result);
		}else{
			for(int i = 0;i < num_peers / sizeof(SceNetAdhocctlPeerInfo);i++){
				pdp_send_postoffice_unicast(idx, &peers[i].mac_addr, dport, data, len, 0, 1);
			}
		}
	}
	_freePeerLock();

	return 0;
}

/**
 * Adhoc Emulator PDP Send Call
 * @param id Socket File Descriptor
 * @param daddr Target MAC Address
 * @param dport Target Port
 * @param data Data Payload
 * @param len Payload Length
 * @param timeout Send Timeout
 * @param flag Nonblocking Flag
 * @return 0 on success or... ADHOC_INVALID_ARG, ADHOC_NOT_INITIALIZED, ADHOC_INVALID_SOCKET_ID, ADHOC_SOCKET_DELETED, ADHOC_INVALID_ADDR, ADHOC_INVALID_PORT, ADHOC_INVALID_DATALEN, ADHOC_SOCKET_ALERTED, ADHOC_TIMEOUT, ADHOC_THREAD_ABORTED, ADHOC_WOULD_BLOCK, NET_NO_SPACE, NET_INTERNAL
 */
int proNetAdhocPdpSend(int id, const SceNetEtherAddr * daddr, uint16_t dport, const void * data, int len, uint32_t timeout, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Port
		if(dport != 0)
		{
			// Valid Data Length
			if(len >= 0) // matching PPSSPP, which allows empty pdp packets
			{
				// Valid Socket ID
				if(id > 0 && id <= 255 && _sockets[id - 1] != NULL && !_sockets[id - 1]->is_ptp)
				{
					// Cast Socket
					SceNetAdhocPdpStat * socket = &_sockets[id - 1]->pdp;
					
					// Valid Data Buffer
					if(data != NULL)
					{
						// Valid Destination Address
						if(daddr != NULL && !_isZeroMac(daddr)) // matching PPSSPP, which drops zero dst
						{
							if (_postoffice){
								return pdp_send_postoffice(id - 1, daddr, dport, data, len, timeout, flag);
							}

							// Log Destination
							#ifdef TRACE
							printk("Attempting PDP Send to %02X:%02X:%02X:%02X:%02X:%02X on Port %u\n", daddr->data[0], daddr->data[1], daddr->data[2], daddr->data[3], daddr->data[4], daddr->data[5], dport);
							#endif
							
							// Schedule Timeout Removal
							if(flag) timeout = 0;

							// Apply Send Timeout Settings to Socket
							sceNetInetSetsockopt(socket->id, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
							
							// Single Target
							if(!_isBroadcastMAC(daddr))
							{
								// Fill in Target Structure
								SceNetInetSockaddrIn target;
								target.sin_family = AF_INET;
								target.sin_port = sceNetHtons(dport);
								
								// Get Peer IP
								if(_resolveMAC((SceNetEtherAddr *)daddr, &target.sin_addr) == 0)
								{
									// Acquire Network Lock
									_acquireNetworkLock();

									// Send Data
									int sent = sceNetInetSendto(socket->id, data, len, ((flag != 0) ? (INET_MSG_DONTWAIT) : (0)), (SceNetInetSockaddr *)&target, sizeof(target));
									
									// Free Network Lock
									_freeNetworkLock();
									
									// Sent Data
									if(sent == len)
									{
										// Success
										return 0;
									}
									
									// Blocking Situation
									if(flag) return ADHOC_WOULD_BLOCK;
									
									// Timeout
									return ADHOC_TIMEOUT;
								}
							}
							
							// Broadcast Target
							else
							{
								// Acquire Network Lock
								_acquireNetworkLock();
								
								#ifdef BROADCAST_TO_LOCALHOST
								// Get Local IP Address
								union SceNetApctlInfo info; if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0)
								{
									// Fill in Target Structure
									SceNetInetSockaddrIn target;
									target.sin_family = AF_INET;
									sceNetInetInetAton(info.ip, &target.sin_addr);
									target.sin_port = sceNetHtons(dport);
									
									// Send Data
									sceNetInetSendto(socket->id, data, len, ((flag != 0) ? (INET_MSG_DONTWAIT) : (0)), (SceNetInetSockaddr *)&target, sizeof(target));
								}
								#endif
								
								// Acquire Peer Lock
								_acquirePeerLock();
								
								// Iterate Peers
								if(!_is_ppsspp){
									SceNetAdhocctlPeerInfoEmu * peer = _getInternalPeerList();
									for(; peer != NULL; peer = peer->next)
									{
										// Fill in Target Structure
										SceNetInetSockaddrIn target;
										target.sin_family = AF_INET;
										target.sin_addr = peer->ip_addr;
										target.sin_port = sceNetHtons(dport);

										// Send Data, nbio, and if the packet buffer can't fit it so be it
										#if NBIO_BCAST
										sceNetInetSendto(socket->id, data, len, INET_MSG_DONTWAIT, (SceNetInetSockaddr *)&target, sizeof(target));
										#else
										sceNetInetSendto(socket->id, data, len, ((flag != 0) ? (INET_MSG_DONTWAIT) : (0)), (SceNetInetSockaddr *)&target, sizeof(target));
										#endif
									}
								}else{
									// haven't figured out adhocctl loading on ppsspp, so here we're dealing with HLE adhocctl
									// making a linked list on emulator side is also a bit meh, so just use the normal peer list, then resolve
									SceNetAdhocctlPeerInfo peers[32];
									int num_peers = sizeof(peers);
									int fetch_result = sceNetAdhocctlGetPeerList(&num_peers, peers);
									if (fetch_result != 0){
										printk("%s: failed fetching peer list in ppsspp mode while trying to broadcast, 0x%x\n", __func__, fetch_result);
									}else{
										for(int i = 0;i < num_peers / sizeof(SceNetAdhocctlPeerInfo);i++){
											SceNetInetSockaddrIn target;
											int resolve_status = _resolveMAC((SceNetEtherAddr *)&peers[i].mac_addr, &target.sin_addr);
											if (resolve_status != 0){
												// printk("%s: failed resolving mac address during broadcast", __func__);
												continue;
											}
											target.sin_family = AF_INET;
											target.sin_port = sceNetHtons(dport);

											// Send Data, nbio, and if the packet buffer can't fit it so be it
											#if NBIO_BCAST
											sceNetInetSendto(socket->id, data, len, INET_MSG_DONTWAIT, (SceNetInetSockaddr *)&target, sizeof(target));
											#else
											sceNetInetSendto(socket->id, data, len, ((flag != 0) ? (INET_MSG_DONTWAIT) : (0)), (SceNetInetSockaddr *)&target, sizeof(target));
											#endif
										}
									}
								}

								// Free Peer Lock
								_freePeerLock();
								
								// Free Network Lock
								_freeNetworkLock();
								
								// Broadcast never fails!
								return 0;
							}
						}
						
						// Invalid Destination Address
						return ADHOC_INVALID_ADDR;
					}
					
					// Invalid Argument
					return ADHOC_INVALID_ARG;
				}
				
				// Invalid Socket ID
				return ADHOC_INVALID_SOCKET_ID;
			}
			
			// Invalid Data Length
			return ADHOC_INVALID_DATALEN;
		}
		
		// Invalid Destination Port
		return ADHOC_INVALID_PORT;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}

/**
 * Broadcast MAC Check
 * @param addr To-be-checked MAC Address
 * @return 1 if Broadcast MAC or... 0
 */
int _isBroadcastMAC(const SceNetEtherAddr * addr)
{
	// Broadcast MAC
	if(memcmp(addr->data, "\xFF\xFF\xFF\xFF\xFF\xFF", ETHER_ADDR_LEN) == 0) return 1;
	
	// Normal MAC
	return 0;
}
