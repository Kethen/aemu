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

int pdp_peek_next_size_postoffice(int idx){
	void *handle = pdp_postoffice_recover(idx);

	if (handle == NULL){
		return 0;
	}

	int next_size = pdp_peek_next_size(handle);
	if (next_size == AEMU_POSTOFFICE_CLIENT_SESSION_DEAD){
		// let next send/recv figure this out
		return 0;
	}

	if (next_size < 0){
		return 0;
	}
	return next_size;
}

/**
 * Adhoc Emulator PDP Socket List Getter
 * @param buflen IN: Length of Buffer in Bytes OUT: Required Length of Buffer in Bytes
 * @param buf PDP Socket List Buffer (can be NULL if you wish to receive Required Length)
 * @return 0 on success or... ADHOC_INVALID_ARG, ADHOC_NOT_INITIALIZED
 */
int proNetAdhocGetPdpStat(int * buflen, SceNetAdhocPdpStat * buf)
{
	// Library is initialized
	if(_init)
	{
		// Length Returner Mode
		if(buflen != NULL && buf == NULL)
		{
			// Return Required Size
			*buflen = sizeof(SceNetAdhocPdpStat) * _getPDPSocketCount();
			
			// Success
			return 0;
		}
		
		// Status Returner Mode
		else if(buflen != NULL && buf != NULL)
		{
			// Socket Count
			int socketcount = _getPDPSocketCount();
			
			// Figure out how many Sockets we will return
			int count = *buflen / sizeof(SceNetAdhocPdpStat);
			if(count > socketcount) count = socketcount;
			
			// Copy Counter
			int i = 0;
			
			// Iterate Translation Table
			int j = 0; for(; j < 255 && i < count; j++)
			{
				// Valid Socket Entry
				if(_sockets[j] != NULL && !_sockets[j]->is_ptp)
				{
					// Copy Socket Data from Internal Memory
					buf[i] = _sockets[j]->pdp;
					
					// Fix Client View Socket ID
					buf[i].id = j + 1;
					
					// Write End of List Reference
					buf[i].next = NULL;

					// Peek udp size, as PPSSPP does in https://github.com/hrydgard/ppsspp/commit/4881f4f0bd0110af5cceeba8dc70f90d0e8d0978
					int udp_size = -1;
					if (_postoffice){
						udp_size = pdp_peek_next_size_postoffice(j);
					}else{
						// we don't care about the content in here, we should not need malloc... I hope
						static uint8_t peek_buf[10 * 1024];
						udp_size = sceNetInetRecv(_sockets[j]->pdp.id, peek_buf, sizeof(peek_buf), INET_MSG_DONTWAIT | INET_MSG_PEEK);
					}
					//printk("%s: udp size %d\n", __func__, udp_size);
					if (udp_size <= 0){
						buf[i].rcv_sb_cc = 0;
					}else{
						buf[i].rcv_sb_cc = udp_size;
					}
					
					// Link Previous Element
					if(i > 0) buf[i - 1].next = &buf[i];
					
					// Increment Counter
					i++;
				}
			}
			
			// Update Buffer Length
			*buflen = i * sizeof(SceNetAdhocPdpStat);
			
			// Success
			return 0;
		}
		
		// Invalid Arguments
		return ADHOC_INVALID_ARG;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}

/**
 * PDP Socket Counter
 * @return Number of internal PDP Sockets
 */
int _getPDPSocketCount(void)
{
	// Socket Counter
	int counter = 0;
	
	// Count Sockets
	int i = 0; for(; i < 255; i++) if(_sockets[i] != NULL && !_sockets[i]->is_ptp) counter++;
	
	// Return Socket Count
	return counter;
}
