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
 * Delete Matching Context
 * @param id Matching Context ID
 * @return 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_INVALID_ID, ADHOC_MATCHING_IS_RUNNING
 */
int proNetAdhocMatchingDelete(int id)
{
	sceKernelLockLwMutex(&context_list_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&context_list_lock, 1); \
		return _v; \
	}

	// Library initialized
	if(_init == 1)
	{
		// Find Matching Context for ID
		SceNetAdhocMatchingContext * context = _findMatchingContext(id);
		
		// Found Matching Context
		if(context != NULL)
		{
			// Context is not running
			if(!context->running)
			{
				// End the pdp socket
				sceNetAdhocPdpDelete(context->socket, 0);

				// Previous Matching Context in Internal List
				SceNetAdhocMatchingContext * previous = NULL;
				
				// Iterate Matching Context List
				SceNetAdhocMatchingContext * item = _contexts; for(; item != NULL; item = item->next)
				{
					// Found Context in List
					if(item == context) break;
					
					// Update Previous Reference
					previous = item;
				}

				// Unlink Context from List
				if(item == context)
				{
					// Middle of List
					if(previous != NULL) previous->next = context->next;
					
					// Beginning of List
					else _contexts = context->next;
				}
				
				// Free Receive Buffer
				_free(context->rxbuf);
				
				// Free Context Buffer
				_free(context);
				
				// Return Success
				RETURN_UNLOCK(0);
			}
			
			// Context is still running
			RETURN_UNLOCK(ADHOC_MATCHING_IS_RUNNING);
		}
		
		// Invalid Matching ID
		RETURN_UNLOCK(ADHOC_MATCHING_INVALID_ID);
	}
	
	// Uninitialized Library
	RETURN_UNLOCK(ADHOC_MATCHING_NOT_INITIALIZED);
}

