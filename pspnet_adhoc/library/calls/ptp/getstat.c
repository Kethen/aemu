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
 * Adhoc Emulator PTP Socket List Getter
 * @param buflen IN: Length of Buffer in Bytes OUT: Required Length of Buffer in Bytes
 * @param buf PTP Socket List Buffer (can be NULL if you wish to receive Required Length)
 * @return 0 on success or... ADHOC_INVALID_ARG, ADHOC_NOT_INITIALIZED
 */
int proNetAdhocGetPtpStat(int * buflen, SceNetAdhocPtpStat * buf)
{
	// Library is initialized
	if(_init)
	{
		// Length Returner Mode
		if(buflen != NULL && buf == NULL)
		{
			// Return Required Size
			*buflen = sizeof(SceNetAdhocPtpStat) * _getPTPSocketCount();
			
			// Success
			return 0;
		}
		
		// Status Returner Mode
		else if(buflen != NULL && buf != NULL)
		{
			// Socket Count
			int socketcount = _getPTPSocketCount();
			
			// Figure out how many Sockets we will return
			int count = *buflen / sizeof(SceNetAdhocPtpStat);
			if(count > socketcount) count = socketcount;
			
			// Copy Counter
			int i = 0;
			
			// Iterate Sockets
			int j = 0; for(; j < 255 && i < count; j++)
			{
				// Active Socket
				if(_sockets[j] != NULL && _sockets[j]->is_ptp)
				{
					// Copy Socket Data from internal Memory
					buf[i] = _sockets[j]->ptp;
					
					// Fix Client View Socket ID
					buf[i].id = j + 1;

					// Peek tcp size, as PPSSPP does in https://github.com/hrydgard/ppsspp/commit/4881f4f0bd0110af5cceeba8dc70f90d0e8d0978
					int tcp_size = 0;
					if (_postoffice){
						if (_sockets[j]->postoffice_handle != NULL){
							tcp_size = ptp_peek_next_size(_sockets[j]->postoffice_handle);
						}
					}else{
						// we don't care about the content in here, we should not need malloc... I hope
						static uint8_t peek_buf[50 * 1024];
						tcp_size = sceNetInetRecv(_sockets[j]->ptp.id, peek_buf, sizeof(peek_buf), INET_MSG_DONTWAIT | INET_MSG_PEEK);
					}
					//printk("%s: tcp size %d\n", __func__, tcp_size);
					if (tcp_size <= 0){
						buf[i].rcv_sb_cc = 0;
					}else{
						buf[i].rcv_sb_cc = tcp_size;
					}
					
					// Write End of List Reference
					buf[i].next = NULL;

					// Link previous Element to this one
					if(i > 0) buf[i-1].next = &buf[i];

					// Increment Counter
					i++;
				}
			}
			
			// Update Buffer Length
			*buflen = i * sizeof(SceNetAdhocPtpStat);
			
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
 * PTP Socket Counter
 * @return Number of internal PTP Sockets
 */
int _getPTPSocketCount(void)
{
	// Socket Counter
	int counter = 0;
	
	// Count Sockets
	int i = 0; for(; i < 255; i++) if(_sockets[i] != NULL && _sockets[i]->is_ptp) counter++;
	
	// Return Socket Count
	return counter;
}
