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

int get_postoffice_fd(int idx){
	AdhocSocket *internal = _sockets[idx];
	void *socket = internal->postoffice_handle;
	if (internal->is_ptp){
		if (socket != NULL){
			if (internal->ptp.state == PTP_STATE_LISTEN){
				return ptp_listen_get_native_sock(socket);
			}else if (internal->ptp.state == PTP_STATE_ESTABLISHED){
				return ptp_get_native_sock(socket);
			}
		}
	}else{
		if (socket != NULL){
			return pdp_get_native_sock(socket);
		}
	}
	return -1;
}

/**
 * Adhoc Emulator PDP Socket Poller (Select Equivalent)
 * @param sds Poll Socket Descriptor Array
 * @param nsds Array Item Count
 * @param timeout Timeout in Microseconds
 * @param flags Nonblocking Flag
 * @return Number of affected Sockets on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_SOCKET_ID, ADHOC_INVALID_ARG, ADHOC_TIMEOUT, ADHOC_THREAD_ABORTED, ADHOC_EXCEPTION_EVENT, NET_NO_SPACE, NET_INTERNAL
 */
int proNetAdhocPollSocket(SceNetAdhocPollSd * sds, int nsds, uint32_t timeout, int flags)
{
	// Library is initialized
	if(_init)
	{
		//printk("%s: polling %d sockets timeout %u flags 0x%x\n", __func__, nsds, timeout, flags);
		// Valid Arguments
		if(sds != NULL && nsds > 0)
		{
			// Socket Check
			int i = 0; for(; i < nsds; i++)
			{
				sds[i].revents = 0;

				// Invalid Socket
				if(sds[i].id < 1 || sds[i].id > 255 || _sockets[sds[i].id - 1] == NULL){
					sds[i].revents = ADHOC_EV_INVALID;
					return ADHOC_INVALID_SOCKET_ID;
				}

				//printk("%s: adhocfd 0x%x fd 0x%x events 0x%x\n", __func__, sds[i].id, _sockets[sds[i].id - 1]->is_ptp ? _sockets[sds[i].id - 1]->ptp.id : _sockets[sds[i].id - 1]->pdp.id, sds[i].events);
			}
			
			// Allocate Infrastructure Memory
			SceNetInetPollfd * isds = (SceNetInetPollfd *)malloc(sizeof(SceNetInetPollfd) * nsds);
			
			// Allocated Memory
			if(isds != NULL)
			{
				// Clear Memory
				memset(isds, 0, sizeof(SceNetInetPollfd) * nsds);

				//printk("%s: nsds %d\n", __func__, nsds);

				// Translate Polling Flags to Infrastructure
				for(i = 0; i < nsds; i++)
				{
					// Fill in Infrastructure Socket ID
					if (_postoffice){
						isds[i].fd = get_postoffice_fd(sds[i].id - 1);
					}else{
						isds[i].fd = _sockets[sds[i].id - 1]->is_ptp ? _sockets[sds[i].id - 1]->ptp.id : _sockets[sds[i].id - 1]->pdp.id;
					}
					
					// Send Event
					if(sds[i].events & ADHOC_EV_SEND) isds[i].events |= INET_POLLWRNORM;
					
					// Receive Event
					if(sds[i].events & ADHOC_EV_RECV) isds[i].events |= INET_POLLRDNORM;

					// check for WRNORM for connect events
					if (sds[i].events & ADHOC_EV_CONNECT && _sockets[sds[i].id - 1]->is_ptp && _sockets[sds[i].id - 1]->ptp.state != PTP_STATE_LISTEN && !_sockets[sds[i].id - 1]->ptp_ext.connect_event_fired){
						isds[i].events |= INET_POLLWRNORM;
					}

					// check for IN for accept events
					if (sds[i].events & ADHOC_EV_ACCEPT && _sockets[sds[i].id - 1]->is_ptp && _sockets[sds[i].id - 1]->ptp.state == PTP_STATE_LISTEN){
						isds[i].events |= INET_POLLIN;
					}

					//printk("%s: adhoc sock 0x%x inet sock 0x%x events 0x%x\n", __func__, sds[i].id, isds[i].fd, isds[i].events);
				}

				//sceKernelDelayThread(1000000);

				int final_timeout = timeout;

				// Nonblocking Mode
				if(flags){
					final_timeout = 0;
					timeout = 0;
				// Timeout Translation (Micro to Milliseconds)
				}else{
					// Convert Timeout
					final_timeout /= 1000;
					
					// Prevent Nonblocking Mode
					if(final_timeout == 0) final_timeout = 1;
				}

				//printk("%s: calling poll with final timeout %u on thread 0x%x with %d free stack\n", __func__, final_timeout, sceKernelGetThreadId(), sceKernelGetThreadStackFreeSize(0));
				
				int affectedsockets = 0;

				// mitigate multi thread poll lock up on the PSP
				uint64_t begin = sceKernelGetSystemTimeWide();
				do{
					// Acquire Network Lock
					_acquireNetworkLock();

					// Poll Sockets
					affectedsockets = sceNetInetPoll(isds, nsds, 0);

					// Free Network Lock
					_freeNetworkLock();

					if (affectedsockets != 0){
						break;
					}
					sceKernelDelayThread(1000);
				}while(sceKernelGetSystemTimeWide() - begin < timeout);
				
				//printk("%s: affected socktes %d\n", __func__, affectedsockets);

				// Sockets affected
				if(affectedsockets > 0)
				{
					// Translate Polling Results to Adhoc
					for(i = 0; i < nsds; i++)
					{
						// In event, gotta hope that listen sockets actually emits In events on the PSP
						if (isds[i].revents & INET_POLLIN){
							if (sds[i].events & ADHOC_EV_ACCEPT && _sockets[sds[i].id - 1]->is_ptp && _sockets[sds[i].id - 1]->ptp.state == PTP_STATE_LISTEN){
								sds[i].revents |= ADHOC_EV_ACCEPT;
							}
						}

						// Send Event
						if(isds[i].revents & INET_POLLWRNORM){
							if (sds[i].events & ADHOC_EV_SEND){
								sds[i].revents |= ADHOC_EV_SEND;
							}
						}
						
						// Receive Event
						if(isds[i].revents & INET_POLLRDNORM){
							if (sds[i].events & ADHOC_EV_RECV){
								sds[i].revents |= ADHOC_EV_RECV;
							}
							if (sds[i].events & ADHOC_EV_CONNECT && _sockets[sds[i].id - 1]->is_ptp && _sockets[sds[i].id - 1]->ptp.state != PTP_STATE_LISTEN && !_sockets[sds[i].id - 1]->ptp_ext.connect_event_fired){
								sds[i].revents |= ADHOC_EV_CONNECT;
								_sockets[sds[i].id - 1]->ptp_ext.connect_event_fired = true;
							}
						}

						// HUP event from the other side
						if (isds[i].revents & INET_POLLHUP){
							sds[i].revents |= ADHOC_EV_DISCONNECT;
						}

						//printk("%s: adhocfd 0x%x fd 0x%x events 0x%x/0x%x revents 0x%x/0x%x\n", __func__, sds[i].id, isds[i].fd, isds[i].events, sds[i].events, isds[i].revents, sds[i].revents);
					}
				}
				
				// Free Memory
				free(isds);
				
				// Blocking Mode (Nonblocking Mode returns 0, even on Success)
				if(!flags)
				{
					// Success
					if(affectedsockets > 0) return affectedsockets;
					
					// Timeout
					return ADHOC_TIMEOUT;
				}
			}
			
			// No Events generated
			return 0;
		}
		
		// Invalid Argument
		return ADHOC_INVALID_ARG;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
