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
 * Delete registered Adhocctl Handler
 * @param id Adhocctl Handler ID
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOCCTL_ID_NOT_FOUND
 */
int proNetAdhocctlDelHandler(int id)
{
	// Library initialized
	if(_init == 1)
	{
		// Valid Arguments
		if(id >= 0 && id < ADHOCCTL_MAX_HANDLER)
		{
			// Valid ID
			if(_event_handler[id] != NULL)
			{
				printk("%s: removing handler id 0x%x 0x%x 0x%x\n", __func__, id, _event_handler[id], _event_args[id]);


				// Clear Event Handler
				_event_handler[id] = NULL;
				_event_args[id] = NULL;

				
				// Return Success
				return 0;
			}
			
			// Invalid ID
			return ADHOCCTL_ID_NOT_FOUND;
		}
		
		// Invalid Arguments
		return ADHOCCTL_INVALID_ARG;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}
