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
 * Adhoc Emulator PTP Connection Acceptor
 * @param id Socket File Descriptor
 * @param addr OUT: Peer MAC Address
 * @param port OUT: Peer Port
 * @param timeout Accept Timeout (in Microseconds)
 * @param flag Nonblocking Flag
 * @return Socket ID >= 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_SOCKET_ID, ADHOC_SOCKET_DELETED, ADHOC_SOCKET_ALERTED, ADHOC_SOCKET_ID_NOT_AVAIL, ADHOC_WOULD_BLOCK, ADHOC_TIMEOUT, ADHOC_NOT_LISTENED, ADHOC_THREAD_ABORTED, NET_INTERNAL
 */
int proNetAdhocPtpAccept(int id, SceNetEtherAddr * addr, uint16_t * port, uint32_t timeout, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Socket
		if(id > 0 && id <= 255 && _sockets[id - 1] != NULL && _sockets[id - 1]->is_ptp)
		{
			// Cast Socket
			SceNetAdhocPtpStat * socket = &_sockets[id - 1]->ptp;
			
			// Listener Socket
			if(socket->state == PTP_STATE_LISTEN)
			{
				// Valid Arguments
				//if(addr != NULL && port != NULL)
				// PPSSPP allows null addr and port for a few games, such as GTA:VCS and Bomberman Panic Bomber
				{
					// Address Information
					SceNetInetSockaddrIn peeraddr;
					memset(&peeraddr, 0, sizeof(peeraddr));
					uint32_t peeraddrlen = sizeof(peeraddr);
					
					// Local Address Information
					SceNetInetSockaddrIn local;
					memset(&local, 0, sizeof(local));
					uint32_t locallen = sizeof(local);
					
					// Grab Nonblocking Flag
					uint32_t nbio = 0;
					uint32_t nbiolen = sizeof(nbio);
					sceNetInetGetsockopt(socket->id, SOL_SOCKET, SO_NBIO, &nbio, &nbiolen);
					
					// Switch to Nonblocking Behaviour
					if(nbio == 0)
					{
						// Overwrite Socket Option
						sceNetInetSetsockopt(socket->id, SOL_SOCKET, SO_NBIO, &_one, sizeof(_one));
					}
					
					// Accept Connection
					int newsocket = sceNetInetAccept(socket->id, (SceNetInetSockaddr *)&peeraddr, &peeraddrlen);
					
					// Blocking Behaviour
					if(!flag && newsocket == -1)
					{
						// Get Start Time
						uint32_t starttime = sceKernelGetSystemTimeLow();
						
						// Retry until Timeout hits
						while((timeout == 0 || (sceKernelGetSystemTimeLow() - starttime) < timeout) && newsocket == -1)
						{
							// Accept Connection
							newsocket = sceNetInetAccept(socket->id, (SceNetInetSockaddr *)&peeraddr, &peeraddrlen);
							
							// Wait a bit...
							sceKernelDelayThread(1000);
						}
					}
					
					// Restore Blocking Behaviour
					if(nbio == 0)
					{
						// Restore Socket Option
						sceNetInetSetsockopt(socket->id, SOL_SOCKET, SO_NBIO, &nbio, sizeof(nbio));
					}
					
					// Accepted New Connection
					if(newsocket > 0)
					{
						// Enable Port Re-use
						sceNetInetSetsockopt(newsocket, SOL_SOCKET, SO_REUSEADDR, &_one, sizeof(_one));
						sceNetInetSetsockopt(newsocket, SOL_SOCKET, SO_REUSEPORT, &_one, sizeof(_one));

						// Enable keep alive like PPSSPP
						sceNetInetSetsockopt(newsocket, SOL_SOCKET, SO_KEEPALIVE, &_one, sizeof(_one));

						// Send faster in case of timeout
						//sceNetInetSetsockopt(newsocket, IPPROTO_TCP, TCP_NODELAY, &_one, sizeof(_one));

						// Grab Local Address
						if(sceNetInetGetsockname(newsocket, (SceNetInetSockaddr *)&local, &locallen) == 0)
						{
							// Peer MAC
							SceNetEtherAddr mac;
							
							// Find Peer MAC
							if(_resolveIP(peeraddr.sin_addr, &mac) == 0)
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
										
										// Copy Socket Descriptor to Structure
										internal->ptp.id = newsocket;
										
										// Copy Local Address Data to Structure
										sceNetGetLocalEtherAddr(&internal->ptp.laddr);
										internal->ptp.lport = sceNetNtohs(local.sin_port);
										
										// Copy Peer Address Data to Structure
										internal->ptp.paddr = mac;
										internal->ptp.pport = sceNetNtohs(peeraddr.sin_port);
										
										// Set Connected State
										internal->ptp.state = PTP_STATE_ESTABLISHED;
										
										// Return Peer Address Information
										if (addr != NULL)
											*addr = internal->ptp.paddr;
										if (port != NULL)
											*port = internal->ptp.pport;
										
										// Link PTP Socket
										_sockets[i] = internal;
										
										// Add Port Forward to Router
										// This should have been opened from listen
										// sceNetPortOpen("TCP", internal->lport);

										_sockets[i]->ptp_ext.mode = PTP_MODE_ACCEPT;

										sceKernelSignalSema(_socket_mapper_mutex, 1);

										// Return Socket
										return i + 1;
									}
									sceKernelSignalSema(_socket_mapper_mutex, 1);
									
									// Free Memory
									free(internal);
								}
							}
						}
						
						// Close Socket
						sceNetInetClose(newsocket);
					}
					
					// Action would block
					if(flag) return ADHOC_WOULD_BLOCK;
					
					// Timeout
					return ADHOC_TIMEOUT;
				}
				
				// Invalid Arguments
				//return ADHOC_INVALID_ARG;
			}
			
			// Client Socket
			return ADHOC_NOT_LISTENED;
		}
		
		// Invalid Socket
		return ADHOC_INVALID_SOCKET_ID;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
