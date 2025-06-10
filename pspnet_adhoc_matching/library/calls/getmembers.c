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
 * Get Member List
 * @return 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_INVALID_ARG, ADHOC_MATCHING_INVALID_ID, ADHOC_MATCHING_NOT_RUNNING
 */
int proNetAdhocMatchingGetMembers(int id, int * buflen, SceNetAdhocMatchingMember * buf)
{
	// Initialized Library
	if(_init == 1)
	{
		// Find Matching Context
		SceNetAdhocMatchingContext * context = _findMatchingContext(id);
		
		// Found Context
		if(context != NULL)
		{
			// Running Context
			if(context->running)
			{
				// Length Buffer available
				if(buflen != NULL)
				{
					// Number of Connected Peers
					uint32_t peercount = _countConnectedPeers(context);
					
					// Calculate Connected Peer Bytesize
					int available = sizeof(SceNetAdhocMatchingMember) * peercount;
					
					// Length Returner Mode
					if(buf == NULL)
					{
						// Get Connected Peer Count
						*buflen = available;
					}
					
					// Normal Mode
					else
					{
						// Fix Negative Length
						if((*buflen) < 0) *buflen = 0;
						
						// Fix Oversize Request
						if((*buflen) > available) *buflen = available;
						
						// Clear Memory
						memset(buf, 0, *buflen);
						
						// Calculate Requested Peer Count
						int requestedpeers = (*buflen) / sizeof(SceNetAdhocMatchingMember);
						
						// Filled Request Counter
						int filledpeers = 0;
						
						// Add Self-Peer
						if(requestedpeers > 0)
						{
							// Add Local MAC
							// P2P mode and Parent mode
							if (peercount == 1 || context->mode != ADHOC_MATCHING_MODE_CHILD)
							{
								buf[filledpeers++].addr = context->mac;
							}
							
							// Room for more than local peer
							if(requestedpeers > 1)
							{
								// P2P Mode
								if(context->mode == ADHOC_MATCHING_MODE_P2P)
								{
									// Find P2P Brother
									SceNetAdhocMatchingMemberInternal * p2p = _findP2P(context);
									
									// P2P Brother found
									if(p2p != NULL)
									{
										// PPSSPP seems to update the ping here
										if (p2p->lastping != 0)
										{
											p2p->lastping = sceKernelGetSystemTimeWide();
										}
										// Add P2P Brother MAC
										buf[filledpeers++].addr = p2p->mac;
									}
								}
								
								// Parent or Child Mode
								else
								{
									// PPSSPP inserts parent here during child mode to force ordering
									// _findParent should not be able to find a parent in parent mode
									SceNetAdhocMatchingMemberInternal *parent = _findParent(context);
									if (parent != NULL)
									{
										if (parent->lastping != 0)
										{
											parent->lastping = sceKernelGetSystemTimeWide();
										}
										buf[filledpeers++].addr = parent->mac;
									}

									// Iterate Peer List
									SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL && filledpeers < requestedpeers; peer = peer->next)
									{
										// PPSSPP bumps lastping here
										if (peer->lastping != 0)
										{
											peer->lastping = sceKernelGetSystemTimeWide();
										}

										// Parent Mode
										if(context->mode == ADHOC_MATCHING_MODE_PARENT)
										{
											// Interested in Children (Michael Jackson Style)
											if(peer->state == ADHOC_MATCHING_PEER_CHILD)
											{
												// Add Child MAC
												buf[filledpeers++].addr = peer->mac;
											}
										}
										
										// Children Mode
										else
										{
											// Interested in Siblings, parent was inserted earlier
											if(peer->state == ADHOC_MATCHING_PEER_CHILD)
											{
												// Add Peer MAC
												buf[filledpeers++].addr = peer->mac;
											}
											else if (peer->state == 0)
											{
												// Insert self like PPSSPP does
												buf[filledpeers++].addr = peer->mac;
											}
										}
									}
								}
								
								// Link Result List
								int i = 0; for(; i < filledpeers - 1; i++)
								{
									// Link Next Element
									buf[i].next = &buf[i + 1];
								}
							}
						}
						
						// Fix Buffer Size
						*buflen = sizeof(SceNetAdhocMatchingMember) * filledpeers;
					}
					
					// Return Success
					return 0;
				}
				
				// Invalid Arguments
				return ADHOC_MATCHING_INVALID_ARG;
			}
			
			// Context not running
			return ADHOC_MATCHING_NOT_RUNNING;
		}
		
		// Invalid Matching ID
		return ADHOC_MATCHING_INVALID_ID;
	}
	
	// Uninitialized Library
	return ADHOC_MATCHING_NOT_INITIALIZED;
}

