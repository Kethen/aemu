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

#include "../common.h"

/**
 * Create Matching Context
 * @param mode Operating Mode (HOST, CLIENT, P2P)
 * @param maxnum Member Limit
 * @param port PDP Port
 * @param rxbuflen Receive Buffer Size
 * @param hello_int Hello Interval (Microseconds)
 * @param keepalive_int Keep-Alive Interval (Microseconds)
 * @param init_count Resend & Keep-Alive Counter Initial Value
 * @param rexmt_int Message Resend Interval (Microseconds)
 * @return ID > 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_INVALID_MAXNUM, ADHOC_MATCHING_RXBUF_TOO_SHORT, ADHOC_MATCHING_INVALID_ARG, ADHOC_MATCHING_NO_SPACE, ADHOC_MATCHING_ID_NOT_AVAIL, ADHOC_MATCHING_PORT_IN_USE
 */
int proNetAdhocMatchingCreate(int mode, int maxnum, uint16_t port, int rxbuflen, uint32_t hello_int, uint32_t keepalive_int, int init_count, uint32_t rexmt_int, SceNetAdhocMatchingHandler handler)
{
	sceKernelLockLwMutex(&context_list_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&context_list_lock, 1); \
		return _v; \
	}

	// Library initialized
	if(_init == 1)
	{
		// Valid Member Limit
		if(maxnum > 1 && maxnum <= 16)
		{
			// Valid Receive Buffer size
			if(rxbuflen >= 1024) // Maybe 2048?
			{
				// Valid Arguments
				if(mode >= 1 && mode <= 3)
				{
					// Iterate Matching Contexts
					SceNetAdhocMatchingContext * item = _contexts; for(; item != NULL; item = item->next)
					{
						// Port Match found
						if(item->port == port) RETURN_UNLOCK(ADHOC_MATCHING_PORT_IN_USE);
					}
					
					// Allocate Context Memory
					SceNetAdhocMatchingContext * context = (SceNetAdhocMatchingContext *)_malloc(sizeof(SceNetAdhocMatchingContext));
					
					// Allocated Memory
					if(context != NULL)
					{
						// Create PDP Socket
						SceNetEtherAddr localmac; sceNetGetLocalEtherAddr(&localmac);
						int socket = sceNetAdhocPdpCreate(&localmac, port, rxbuflen, 0);
						
						// Created PDP Socket
						if(socket > 0)
						{
							// Clear Memory
							memset(context, 0, sizeof(SceNetAdhocMatchingContext));
						
							// Allocate Receive Buffer
							context->rxbuf = (uint8_t *)_malloc(rxbuflen);
						
							// Allocated Memory
							if(context->rxbuf != NULL)
							{
								// Clear Memory
								memset(context->rxbuf, 0, rxbuflen);
								
								// Fill in Context Data
								context->id = _findFreeMatchingID();
								context->mode = mode;	
								context->maxpeers = maxnum;
								context->port = port;
								context->socket = socket;
								context->rxbuflen = rxbuflen;
								context->hello_int = hello_int;
								context->keepalive_int = 500000;
								//context->keepalive_int = keepalive_int;
								context->resendcounter = init_count;
								context->keepalivecounter = 100;
								//context->keepalivecounter = init_count;
								context->resend_int = rexmt_int;
								context->handler = handler;
								
								// Fill in Selfpeer
								context->mac = localmac;
								
								// Link Context
								context->next = _contexts;
								_contexts = context;
								
								// Return Matching ID
								RETURN_UNLOCK(context->id);
							}
							
							// Close PDP Socket
							sceNetAdhocPdpDelete(socket, 0);
						}
						
						// Free Memory
						_free(context);
						
						// Port in use
						if(socket < 1) RETURN_UNLOCK(ADHOC_MATCHING_PORT_IN_USE);
					}
					
					// Out of Memory
					RETURN_UNLOCK(ADHOC_MATCHING_NO_SPACE);
				}
				
				// Invalid Arguments
				RETURN_UNLOCK(ADHOC_MATCHING_INVALID_ARG);
			}
			
			// Invalid Receive Buffer Size
			RETURN_UNLOCK(ADHOC_MATCHING_RXBUF_TOO_SHORT);
		}
		
		// Invalid Member Limit
		RETURN_UNLOCK(ADHOC_MATCHING_INVALID_MAXNUM);
	}
	
	// Uninitialized Library
	RETURN_UNLOCK(ADHOC_MATCHING_NOT_INITIALIZED);
}

