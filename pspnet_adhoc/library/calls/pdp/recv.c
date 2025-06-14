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
 * Adhoc Emulator PDP Receive Call
 * @param id Socket File Descriptor
 * @param saddr OUT: Source MAC Address
 * @param sport OUT: Source Port
 * @param buf OUT: Received Data
 * @param len IN: Buffer Size OUT: Received Data Length
 * @param timeout Receive Timeout
 * @param flag Nonblocking Flag
 * @return 0 on success or... ADHOC_INVALID_ARG, ADHOC_NOT_INITIALIZED, ADHOC_INVALID_SOCKET_ID, ADHOC_SOCKET_DELETED, ADHOC_SOCKET_ALERTED, ADHOC_WOULD_BLOCK, ADHOC_TIMEOUT, ADHOC_NOT_ENOUGH_SPACE, ADHOC_THREAD_ABORTED, NET_INTERNAL
 */
int proNetAdhocPdpRecv(int id, SceNetEtherAddr * saddr, uint16_t * sport, void * buf, int * len, uint32_t timeout, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Socket ID
		if(id > 0 && id <= 255 && _pdp[id - 1] != NULL)
		{
			// Cast Socket
			SceNetAdhocPdpStat * socket = _pdp[id - 1];
			
			// Valid Arguments
			if(saddr != NULL && sport != NULL && buf != NULL && len != NULL && *len > 0)
			{
				#ifndef PDP_DIRTY_MAGIC
				// Schedule Timeout Removal
				if(flag) timeout = 0;
				#else
				// Nonblocking Simulator
				int wouldblock = 0;
				
				// Minimum Timeout
				uint32_t mintimeout = 250000;
				
				// Nonblocking Call
				if(flag)
				{
					// Erase Nonblocking Flag
					flag = 0;
					
					// Set Wouldblock Behaviour
					wouldblock = 1;
					
					// Set Minimum Timeout (250ms)
					if(timeout < mintimeout) timeout = mintimeout;
				}
				#endif
				
				// Apply Receive Timeout Settings to Socket
				sceNetInetSetsockopt(socket->id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
				
				// Sender Address
				SceNetInetSockaddrIn sin;
				
				// Set Address Length (so we get the sender ip)
				uint32_t sinlen = sizeof(sin);
				sin.sin_len = (uint8_t)sinlen;
				
				// Acquire Network Lock
				_acquireNetworkLock();
				
				// Receive Data
				// Detect oversize packets, as PPSSPP does
				void *recv_buf = malloc(*len + 1);
				int received = 0;
				if (__builtin_expect(recv_buf != NULL, 1))
				{
					received = sceNetInetRecvfrom(socket->id, recv_buf, *len + 1, ((flag != 0) ? (INET_MSG_DONTWAIT) : (0)), (SceNetInetSockaddr *)&sin, &sinlen);
				}
				else
				{
					printk("%s: failed allocating size test buffer\n", __func__);
					received = sceNetInetRecvfrom(socket->id, buf, *len, ((flag != 0) ? (INET_MSG_DONTWAIT) : (0)), (SceNetInetSockaddr *)&sin, &sinlen);
				}
				
				// Received Data
				if(received > 0)
				{
					// Peer MAC
					SceNetEtherAddr mac;

					// Find Peer MAC
					if(_resolveIP(sin.sin_addr, &mac) == 0)
					{
						// Free Network Lock
						_freeNetworkLock();

						// Provide Sender Information
						*saddr = mac;
						*sport = sceNetNtohs(sin.sin_port);

						int ret = 0;
						int recv_len = received;

						if (__builtin_expect(received <= *len, 1))
						{
							// Save Length
							*len = received;
						}
						else
						{
							// The packet is bigger than expected
							ret = ADHOC_NOT_ENOUGH_SPACE;
							recv_len = *len;
						}

						if (__builtin_expect(recv_buf != NULL, 1))
						{
							memcpy(buf, recv_buf, recv_len);
							free(recv_buf);
						}

						// Return Success
						return ret;
					}
				}

				if (received == -1)
				{
					int inet_error = sceNetInetGetErrno();
					if (inet_error != EAGAIN)
					{
						printk("%s: returning timeout with inet error 0x%x\n", __func__, inet_error);
					}
				}

				// Free Network Lock
				_freeNetworkLock();

				if (__builtin_expect(recv_buf != NULL, 1))
				{
					free(recv_buf);
				}
				
				#ifdef PDP_DIRTY_MAGIC
				// Restore Nonblocking Flag for Return Value
				if(wouldblock) flag = 1;
				#endif
				
				// Nothing received
				if(flag) return ADHOC_WOULD_BLOCK;
				return ADHOC_TIMEOUT;
			}
			
			// Invalid Argument
			return ADHOC_INVALID_ARG;
		}
		
		// Invalid Socket ID
		return ADHOC_INVALID_SOCKET_ID;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
