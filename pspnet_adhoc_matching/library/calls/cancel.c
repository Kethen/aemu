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
 * Cancel / Kick / Leave Matching Target
 * @param id Matching Context ID
 * @param target Target MAC
 * @param optlen Length of Optional Data
 * @param opt Optional Data
 * @return 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_INVALID_ARG, ADHOC_MATCHING_INVALID_ID, ADHOC_MATCHING_NOT_RUNNING, ADHOC_MATCHING_UNKNOWN_TARGET, ADHOC_MATCHING_NO_SPACE
 */
int proNetAdhocMatchingCancelTargetWithOpt(int id, const SceNetEtherAddr * target, int optlen, const void * opt)
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
		if(target != NULL && ((optlen == 0 && opt == NULL) || (optlen > 0 && opt != NULL)))
		{
			// Find Matching Context
			SceNetAdhocMatchingContext * context = _findMatchingContext(id);
			
			// Found Matching Context
			if(context != NULL)
			{
				// Running Context
				if(context->running)
				{
					// Find Peer
					SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, (SceNetEtherAddr *)target);
					
					// Found Peer
					if(peer != NULL)
					{
						// Valid Peer Mode
						if((context->mode == ADHOC_MATCHING_MODE_CHILD && (peer->state == ADHOC_MATCHING_PEER_PARENT || peer->state == ADHOC_MATCHING_PEER_OUTGOING_REQUEST)) || 
						(context->mode == ADHOC_MATCHING_MODE_PARENT && (peer->state == ADHOC_MATCHING_PEER_CHILD || peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST)) || 
						(context->mode == ADHOC_MATCHING_MODE_P2P && (peer->state == ADHOC_MATCHING_PEER_P2P || peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST)))
						{
							// Notify other Children of Death
							if(context->mode == ADHOC_MATCHING_MODE_PARENT && peer->state == ADHOC_MATCHING_PEER_CHILD && _countConnectedPeers(context) > 1)
							{
								// Send Death Message
								_sendDeathMessage(context, peer);
							}
							
							// Mark Peer as Canceled
							peer->state = ADHOC_MATCHING_PEER_CANCEL_IN_PROGRESS;
							
							// Send Cancel Event to Peer
							_sendCancelMessage(context, peer, optlen, opt);
							
							// Delete Peer from List
							// Can't delete here, Threads need this data still.
							// _deletePeer(context, peer);
							
							// Return Success
							UNLOCK_RETURN(0);
						}
					}
					
					// Peer not found
					UNLOCK_RETURN(ADHOC_MATCHING_UNKNOWN_TARGET);
				}
				
				// Context not running
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

