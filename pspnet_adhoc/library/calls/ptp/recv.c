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
 * Adhoc Emulator PTP Receiver
 * @param id Socket File Descriptor
 * @param buf Data Buffer
 * @param len IN: Buffersize OUT: Received Data (in Bytes)
 * @param timeout Receive Timeout (in Microseconds)
 * @param flag Nonblocking Flag
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_SOCKET_ID, ADHOC_SOCKET_DELETED, ADHOC_SOCKET_ALERTED, ADHOC_WOULD_BLOCK, ADHOC_TIMEOUT, ADHOC_THREAD_ABORTED, ADHOC_DISCONNECTED, NET_INTERNAL
 */
int proNetAdhocPtpRecv(int id, void * buf, int * len, uint32_t timeout, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Socket
		if(id > 0 && id <= 255 && _sockets[id - 1] != NULL && _sockets[id - 1]->is_ptp && _sockets[id - 1]->ptp.state == PTP_STATE_ESTABLISHED)
		{
			// Cast Socket
			SceNetAdhocPtpStat * socket = &_sockets[id - 1]->ptp;
			
			// Valid Arguments
			if(buf != NULL && len != NULL && *len > 0)
			{
				// Schedule Timeout Removal
				if(flag) timeout = 0;
				
				// Apply Receive Timeout Settings to Socket
				sceNetInetSetsockopt(socket->id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
				
				// Acquire Network Lock
				_acquireNetworkLock();
				
				// Receive Data
				int received = sceNetInetRecv(socket->id, buf, *len, ((flag) ? (INET_MSG_DONTWAIT) : (0)));
				
				// Free Network Lock
				_freeNetworkLock();
				
				// Received Data
				if(received > 0)
				{
					// Save Length
					*len = received;
					
					// Return Success
					return 0;
				}
				
				// Non-Critical Error
				else if(received == -1 && sceNetInetGetErrno() == EAGAIN)
				{
					// Blocking Situation
					if(flag) return ADHOC_WOULD_BLOCK;
					
					// Timeout
					return ADHOC_TIMEOUT;
				}
				
				// Change Socket State
				socket->state = PTP_STATE_CLOSED;
				
				// Disconnected
				return ADHOC_DISCONNECTED;
			}
			
			// Invalid Arguments
			return ADHOC_INVALID_ARG;
		}
		
		// Invalid Socket
		return ADHOC_INVALID_SOCKET_ID;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
