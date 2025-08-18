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

/**
 * Adhoc Emulator PDP Socket Delete
 * @param id Socket File Descriptor
 * @param flag Bitflags (Unused)
 * @return 0 on success or... ADHOC_INVALID_ARG, ADHOC_NOT_INITIALIZED, ADHOC_INVALID_SOCKET_ID
 */
int proNetAdhocPdpDelete(int id, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Arguments
		if(id > 0 && id <= 255)
		{
			// Log Delete Attempt
			#ifdef DEBUG
			printk("Attempting to Delete Socket %d\n", id);
			#endif
			
			// Cast Socket
			SceNetAdhocPdpStat *socket = NULL;
			if (_sockets[id - 1] != NULL && !_sockets[id - 1]->is_ptp){
				socket = &_sockets[id - 1]->pdp;
			}
			
			// Valid Socket
			if(socket != NULL)
			{
				// Close Connection
				sceNetInetClose(socket->id);
				
				// Remove Port Forward from Router
				sceNetPortClose("UDP", socket->lport);
				
				// Free Memory
				free(_sockets[id - 1]);
				
				// Free Translation Slot
				sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
				_sockets[id - 1] = NULL;
				sceKernelSignalSema(_socket_mapper_mutex, 1);
				
				// Success
				return 0;
			}
			
			// Invalid Socket ID
			return ADHOC_INVALID_SOCKET_ID;
		}
		
		// Invalid Argument
		return ADHOC_INVALID_ARG;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
