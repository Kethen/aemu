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
 * Register (up to 4) Adhocctl Event Handler
 * @param handler To-be-registered Adhocctl Handler
 * @param arg Optional to-be-passed Parameter (Get's passed to Handler in case of Event)
 * @return Handler ID > 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOCCTL_HANDLER_MAX
 */
int proNetAdhocctlAddHandler(SceNetAdhocctlHandler handler, void * arg)
{
	// Library initialized
	if(_init == 1)
	{
		// Valid Arguments
		if(handler != NULL)
		{
			// Find free Handler Slot
			int i = 0; for(; i < ADHOCCTL_MAX_HANDLER; i++)
			{
				// Found free Slot
				if(_event_handler[i] == NULL)
				{
					// Register Handler
					_event_handler[i] = handler;
					_event_args[i] = arg;
					_event_handler_gp[i] = get_gp_value();

					printk("%s: added handler %d 0x%x 0x%x with gp 0x%x\n", __func__, i + 1, handler, arg, _event_handler_gp[i]);

					// Return Handler ID
					return i + 1;
				}
			}
			
			// Out of Slots
			return ADHOCCTL_HANDLER_MAX;
		}
		
		// Invalid Arguments
		return ADHOCCTL_INVALID_ARG;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}
