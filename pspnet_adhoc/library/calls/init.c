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

// sockets
AdhocSocket *_sockets[255];

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

int rehook_inet();

int _postoffice = 0;
int _vita_speedup = 0;

static int postoffice_handle = -1;

// kernel module assisted load
void load_inet_modules();

int _game_initialized_inet = 0;

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
		if (_is_ppsspp){
			ppsspp_return_memory();
		}

		_vita_speedup = rehook_inet() == 0;

		if (_vita_speedup){
			printk("%s: vita speedup detected\n", __func__);
		}else{
			printk("%s: vita speedup not detected\n", __func__);
		}

		// precache port offset
		get_port_offset();

		int result = 0;

		// Load Internet Modules
		#if 0
		result = sceUtilityLoadModule(PSP_MODULE_NET_INET);
		printk("%s: loading internet modules, 0x%x\n", __func__, result);
		#else
		load_inet_modules();
		#endif

		SceKernelLMOption mod_load_high_option = {
			.size = sizeof(SceKernelLMOption),
			.mpidtext = partition_to_use(),
			.mpiddata = partition_to_use(),
			.flags = 0,
			.position = PSP_SMEM_High,
			.access = 0,
			.creserved = {0, 0}
		};

		// Load postoffice lib
		if (postoffice_handle < 0){
			postoffice_handle = sceKernelLoadModule("ms0:/kd/aemu_postoffice.prx", 0, &mod_load_high_option);
			if (postoffice_handle < 0){
				postoffice_handle = sceKernelLoadModule("ms0:/PSP/PLUGINS/atpro/aemu_postoffice.prx", 0, NULL);
			}
			if (postoffice_handle >= 0){
				int start_status = sceKernelStartModule(postoffice_handle, 0, NULL, NULL, NULL);
				if (start_status >= 0){
					_postoffice = 1;
					printk("%s: postoffice prx loaded, enabling postoffice mode\n", __func__);
				}else{
					printk("%s: failed starting aemu postoffice, 0x%x\n", __func__, postoffice_handle);
				}
			}else{
				printk("%s: failed loading aemu postoffice, 0x%x\n", __func__, postoffice_handle);
			}
		}

		// Enable Manual Infrastructure Module Control
		_manage_modules = 1;

		// Initialize Internet Library
		int inet_init_status = 0;
		if (_game_initialized_inet){
			printk("%s: corwardly not initializing inet again since the game did it\n", __func__);
		} else {
			inet_init_status = sceNetInetInit();
			printk("%s: sceNetInetInit(), 0x%x\n", __func__, inet_init_status);
		}

		if (inet_init_status != 0){
			if (*(uint32_t *)&inet_init_status == 0x80410201){
				printk("%s: game initialized inet already, there be dragons\n", __func__);
				_game_initialized_inet = 1;
			} else {
				return -1;
			}
		}

		// redo inet hooks
		rehook_inet();

		// Clear Translator Memory
		memset(&_sockets, 0, sizeof(_sockets));

		// Library initialized
		_init = 1;

		// Return Success
		return 0;
	}
	
	// Already initialized
	return ADHOC_ALREADY_INITIALIZED;
}
