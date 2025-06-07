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
 * Adhoc Emulator PTP Connection Opener
 * @param id Socket File Descriptor
 * @param timeout Connect Timeout (in Microseconds)
 * @param flag Nonblocking Flag
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_SOCKET_ID, ADHOC_SOCKET_DELETED, ADHOC_CONNECTION_REFUSED, ADHOC_SOCKET_ALERTED, ADHOC_WOULD_BLOCK, ADHOC_TIMEOUT, ADHOC_NOT_OPENED, ADHOC_THREAD_ABORTED, NET_INTERNAL
 */
int proNetAdhocPtpConnect(int id, uint32_t timeout, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Socket
		if(id > 0 && id <= 255 && _ptp[id - 1] != NULL)
		{
			// Cast Socket
			SceNetAdhocPtpStat * socket = _ptp[id - 1];

			// PPSSPP's Phantasy Star Portable 2 fix, allow re-calling connect
			if (socket->state == PTP_STATE_ESTABLISHED) 
			{
				return 0;
			}
			
			// Valid Client Socket
			if(socket->state == 0)
			{
				// Target Address
				SceNetInetSockaddrIn sin;
				memset(&sin, 0, sizeof(sin));
				
				// Setup Target Address
				sin.sin_len = sizeof(sin);
				sin.sin_family = AF_INET;
				sin.sin_port = sceNetHtons(socket->pport);
				
				// Grab Peer IP
				if(_resolveMAC(&socket->paddr, &sin.sin_addr) == 0)
				{
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
					
					// Connect Socket to Peer (Nonblocking)
					int connectresult = sceNetInetConnect(socket->id, (SceNetInetSockaddr *)&sin, sizeof(sin));
					
					// Grab Error Code
					int errorcode = sceNetInetGetErrno();
					
					// Restore Blocking Behaviour
					if(nbio == 0)
					{
						// Restore Socket Option
						sceNetInetSetsockopt(socket->id, SOL_SOCKET, SO_NBIO, &nbio, sizeof(nbio));
					}
					
					// Instant Connection (Lucky!)
					if(connectresult == 0 || (connectresult == -1 && errorcode == EISCONN))
					{
						// Set Connected State
						socket->state = PTP_STATE_ESTABLISHED;
						
						// Success
						return 0;
					}
					
					// Connection in Progress
					else if(connectresult == -1 && errorcode == EINPROGRESS)
					{
						// Nonblocking Mode
						if(flag) return ADHOC_WOULD_BLOCK;
						
						// Blocking Mode
						else
						{
							// Grab Connection Start Time
							uint32_t starttime = sceKernelGetSystemTimeLow();
							
							// Peer Information (for Connection-Polling)
							SceNetInetSockaddrIn peer;
							memset(&peer, 0, sizeof(peer));
							uint32_t peerlen = sizeof(peer);
							
							// Wait for Connection
							while((timeout == 0 || (sceKernelGetSystemTimeLow() - starttime) < timeout) && sceNetInetGetpeername(socket->id, (SceNetInetSockaddr *)&peer, &peerlen) != 0)
							{
								// Wait 1ms
								sceKernelDelayThread(1000);
							}
							
							// Connected in Time
							if(sin.sin_addr == peer.sin_addr/* && sin.sin_port == peer.sin_port*/)
							{
								// Set Connected State
								socket->state = PTP_STATE_ESTABLISHED;
								
								// Success
								return 0;
							}
							
							// Timeout occured
							return ADHOC_TIMEOUT;
						}
					}
					// Cannot connect for any other reasons
					return ADHOC_CONNECTION_REFUSED;
				}
				// Peer not found
				//return ADHOC_CONNECTION_REFUSED;
				// PPSSPP returns this instead here
				return ADHOC_INVALID_ADDR;
			}
			
			// Not a valid Client Socket
			return ADHOC_NOT_OPENED;
		}
		
		// Invalid Socket
		return ADHOC_INVALID_SOCKET_ID;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
