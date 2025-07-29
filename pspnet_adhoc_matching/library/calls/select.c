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
 * Select / Accept Matching Target
 * @param id Matching Context ID
 * @param target Target MAC
 * @param optlen Length of Optional Data
 * @param opt Optional Data
 * @return 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_INVALID_ARG, ADHOC_MATCHING_INVALID_ID, ADHOC_MATCHING_NOT_RUNNING, ADHOC_MATCHING_UNKNOWN_TARGET, ADHOC_MATCHING_INVALID_OPTLEN, ADHOC_MATCHING_TARGET_NOT_READY, ADHOC_MATCHING_EXCEED_MAXNUM, ADHOC_MATCHING_NO_SPACE, ADHOC_MATCHING_REQUEST_IN_PROGRESS, ADHOC_MATCHING_ALREADY_ESTABLISHED
 */
int proNetAdhocMatchingSelectTarget(int id, const SceNetEtherAddr * target, int optlen, const void * opt)
{
	sceKernelLockLwMutex(&context_list_lock, 1, 0);
	sceKernelLockLwMutex(&members_lock, 1, 0);
	#define UNLOCK_RETURN(_v) { \
		sceKernelUnlockLwMutex(&context_list_lock, 1); \
		sceKernelUnlockLwMutex(&members_lock, 1); \
		return _v; \
	}

	// Initialized Library
	if(_init == 1)
	{
		// Valid Arguments
		if(target != NULL)
		{
			// Find Matching Context for ID
			SceNetAdhocMatchingContext * context = _findMatchingContext(id);
			
			// Found Matching Context
			if(context != NULL)
			{
				// Running Context
				if(context->running)
				{
					// Search Result
					SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, (SceNetEtherAddr *)target);
					
					// Found Peer in List
					if(peer != NULL)
					{
						// Valid Optional Data Length
						if((optlen == 0 && opt == NULL) || (optlen > 0 && opt != NULL))
						{
							// Host Mode
							if(context->mode == ADHOC_MATCHING_MODE_PARENT)
							{
								// Already Connected
								if(peer->state == ADHOC_MATCHING_PEER_CHILD) UNLOCK_RETURN(ADHOC_MATCHING_ALREADY_ESTABLISHED);
								
								// Not enough space
								if(_countChildren(context) == (context->maxpeers - 1)) UNLOCK_RETURN(ADHOC_MATCHING_EXCEED_MAXNUM);
								
								// Requesting Peer
								if(peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST)
								{
									// Accept Peer in Group
									peer->state = ADHOC_MATCHING_PEER_CHILD;

									context->input_stack_reverse_lock = 1;
									// Send Accept Confirmation to Peer
									_sendAcceptMessage(context, peer, optlen, opt);
									
									// Tell Children about new Sibling
									_sendBirthMessage(context, peer);
									context->input_stack_reverse_lock = 0;
									
									// Return Success
									UNLOCK_RETURN(0);
								}
							}
							
							// Client Mode
							else if(context->mode == ADHOC_MATCHING_MODE_CHILD)
							{
								// Already connected
								if(_findParent(context) != NULL) UNLOCK_RETURN(ADHOC_MATCHING_ALREADY_ESTABLISHED);
								
								// Outgoing Request in Progress
								if(_findOutgoingRequest(context) != NULL) UNLOCK_RETURN(ADHOC_MATCHING_REQUEST_IN_PROGRESS);
								
								// Valid Offer
								if(peer->state == ADHOC_MATCHING_PEER_OFFER)
								{
									// Switch into Join Request Mode
									peer->state = ADHOC_MATCHING_PEER_OUTGOING_REQUEST;
									
									// Send Join Request to Peer
									_sendJoinRequest(context, peer, optlen, opt);
									
									// Return Success
									UNLOCK_RETURN(0);
								}
							}
							
							// P2P Mode
							else
							{
								// Already connected
								if(_findP2P(context) != NULL) UNLOCK_RETURN(ADHOC_MATCHING_ALREADY_ESTABLISHED);
								
								// Outgoing Request in Progress
								if(_findOutgoingRequest(context) != NULL) UNLOCK_RETURN(ADHOC_MATCHING_REQUEST_IN_PROGRESS);
								
								// Join Request Mode
								if(peer->state == ADHOC_MATCHING_PEER_OFFER)
								{
									// Switch into Join Request Mode
									peer->state = ADHOC_MATCHING_PEER_OUTGOING_REQUEST;
									
									// Send Join Request to Peer
									_sendJoinRequest(context, peer, optlen, opt);
									
									// Return Success
									UNLOCK_RETURN(0);
								}
								
								// Requesting Peer
								else if(peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST)
								{
									// Accept Peer in Group
									peer->state = ADHOC_MATCHING_PEER_P2P;
									
									// Send Accept Confirmation to Peer
									_sendAcceptMessage(context, peer, optlen, opt);
									
									// Return Success
									UNLOCK_RETURN(0);
								}
							}
							
							// How did this happen?! It shouldn't!
							UNLOCK_RETURN(ADHOC_MATCHING_TARGET_NOT_READY);
						}
						
						// Invalid Optional Data Length
						UNLOCK_RETURN(ADHOC_MATCHING_INVALID_OPTLEN);
					}
					
					// Peer not found
					UNLOCK_RETURN(ADHOC_MATCHING_UNKNOWN_TARGET);
				}
				
				// Idle Context
				UNLOCK_RETURN(ADHOC_MATCHING_NOT_RUNNING);
			}
			
			// Invalid Matching ID
			UNLOCK_RETURN(ADHOC_MATCHING_INVALID_ID);
		}
		
		// Invalid Arguments
		UNLOCK_RETURN(ADHOC_MATCHING_INVALID_ARG);
	}
	
	// Uninitialized Library
	UNLOCK_RETURN(ADHOC_MATCHING_NOT_INITIALIZED);
}

