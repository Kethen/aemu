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

// PDP Sockets
SceNetAdhocPdpStat * _pdp[255];

// PTP Sockets
SceNetAdhocPtpStat * _ptp[255];

// Gamemode Buffer
SceNetAdhocGameModeBufferStat * _gmb = NULL;

// Initialized Switch
int _init = 0;

// Manage Infrastructure Modules Switch
int _manage_modules = 0;

// Global One
int _one = 1;

// Global Zero
int _zero = 0;

/**
 * Adhoc Emulator Socket Library Init-Call
 * @return 0 on success or... ADHOC_ALREADY_INITIALIZED
 */
int proNetAdhocInit(void)
{

	// Library uninitialized
	if(!_init)
	{
		return_memory();
		// Initialize Internet Library
		int result = sceNetInetInit();
		
		// Missing Internet Modules
		if(result != 0)
		{
			// Load Internet Modules
			sceUtilityLoadModule(PSP_MODULE_NET_INET);
			
			// Enable Manual Infrastructure Module Control
			_manage_modules = 1;
			
			// Re-Initialize Internet Library
			result = sceNetInetInit();
		}

		if(result != 0)
		{
			steal_memory();
			printk("%s: failed to initialize internet lib, 0x%x\n", __func__, result);
		}

		// Initialized Internet Library
		if(result == 0)
		{
			// Clear Translator Memory
			memset(&_pdp, 0, sizeof(_pdp));
			memset(&_ptp, 0, sizeof(_ptp));
			
			// Library initialized
			_init = 1;
			
			// Return Success
			return 0;
		}
		
		// Generic Error
		return -1;
	}
	
	// Already initialized
	return ADHOC_ALREADY_INITIALIZED;
}
