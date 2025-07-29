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
 * Stop Matching Context
 * @param id Matching Context ID
 * @return 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_INVALID_ID
 */
int proNetAdhocMatchingStop(int id)
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
			// Context is running
			if(context->running)
			{
				// DO NOT CHANGE THE THREAD SHUTDOWN ORDER!
				// MATCHING FIRES A BYE EVENT CAUSED BY THE IO THREAD SHUTDOWN THAT THE EVENT THREAD HAS TO CATCH!

				uint64_t now = sceKernelGetSystemTimeWide();
				uint64_t diff = now - last_packet_send;
				static const uint64_t grace_period = 500000;
				if (diff < grace_period){
					uint64_t delay = grace_period - diff;
					printk("%s: holding up matching destruction for %d usec for matching packets\n", __func__, delay);
					sceKernelDelayThread(delay);
				}

				asm volatile("": : :"memory");
				
				// Shutdown IO Thread
				context->input_thid = -1;
				
				// Wait for Shutdown
				while(context->input_thid != 0) sceKernelDelayThread(10000);
				
				// Shutdown Event Dispatcher Thread
				context->event_thid = -1;
				
				// Wait for Shutdown
				while(context->event_thid != 0) sceKernelDelayThread(10000);
				
				// Free Hello Data Memory
				_free(context->hello);
				
				// Erase Hello Data Info
				context->hellolen = 0;
				context->hello = NULL;
				
				// Set Context into Stopped State
				context->running = 0;
			}
			
			// Return Success
			RETURN_UNLOCK(0);
		}
		
		// Invalid Matching ID
		RETURN_UNLOCK(ADHOC_MATCHING_INVALID_ID);
	}
	
	// Uninitialized Library
	RETURN_UNLOCK(ADHOC_MATCHING_NOT_INITIALIZED);
}

