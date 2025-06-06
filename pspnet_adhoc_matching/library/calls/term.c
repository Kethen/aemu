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
 * Terminate Adhoc Matching Emulator
 * @return 0 on success or... ADHOC_MATCHING_BUSY
 */
int proNetAdhocMatchingTerm(void)
{
	sceKernelLockLwMutex(&context_list_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&context_list_lock, 1); \
		return _v; \
	}

	// Library initialized
	if(_init == 1)
	{
		// Stop and delete all contexts
		while(_contexts != NULL)
		{
			printk("%s: removing %d 0x%x\n", __func__, contexts->id, _contexts);
			proNetAdhocMatchingStop(_contexts->id);
			proNetAdhocMatchingDelete(_contexts->id);
		}

		// Mark Library as shutdown
		_init = 0;

		// Clear Fake Pool Size
		_fake_poolsize = 0;

		#if 0
		// No Context alive
		if(_contexts == NULL)
		{
			// Mark Library as shutdown
			_init = 0;
			
			// Clear Fake Pool Size
			_fake_poolsize = 0;
		}
		
		// Library is busy
		else return ADHOC_MATCHING_BUSY;
		#endif
	}
	
	// Return Success
	RETURN_UNLOCK(0);
}

