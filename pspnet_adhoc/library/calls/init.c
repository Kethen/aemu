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

		load_inet_modules();

		// Initialize Internet Library
		int result = sceNetInetInit();

		if(result != 0)
		{
			steal_memory();
			printk("%s: failed initialize internet library\n", __func__);
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

static const char *inet_modules[] = {
	"flash0:/kd/pspnet_inet.prx",
	"flash0:/kd/pspnet_apctl.prx",
	"flash0:/kd/pspnet_resolver.prx"
};

static const int num_inet_modules = sizeof(inet_modules) / sizeof(char *);

static SceUID inet_modids[] = {-1, -1, -1};

SceUID kuKernelLoadModule(const char *path, int flags, SceKernelLMOption *option);

void load_inet_modules()
{
	for (int i = 0;i < num_inet_modules;i++)
	{
		if (inet_modids[i] >= 0)
		{
			printk("%s: %s is already loaded\n", __func__, inet_modules[i]);
			continue;
		}

		// Loading into p5 here would usually just crash, current theory is that inet uses p5 without partition allocation
		SceKernelLMOption load_options = {0};
		load_options.size = sizeof(SceKernelLMOption);
		load_options.mpidtext = 2;
		load_options.mpiddata = 2;
		load_options.position = PSP_SMEM_Low;
		load_options.access = 1;

		inet_modids[i] = kuKernelLoadModule(inet_modules[i], 0, &load_options);
		if (inet_modids[i] < 0)
		{
			printk("%s: failed loading %s, 0x%x\n", __func__, inet_modules[i], inet_modids[i]);
			continue;
		}
		int result = 0;
		int start_status = sceKernelStartModule(inet_modids[i], 0, NULL, &result, NULL);
		if (start_status < 0)
		{
			printk("%s: failed starting %s, 0x%x\n", __func__, inet_modules[i], start_status);
			int unload_status = sceKernelUnloadModule(inet_modids[i]);
			if (unload_status < 0)
			{
				printk("%s: failed unloading module after failed starting module\n", __func__);
			}
			else
			{
				inet_modids[i] = -1;
			}
			continue;
		}
		printk("%s: loaded and started %s\n", __func__, inet_modules[i]);
	}
}

void unload_inet_modules()
{
	for (int i = num_inet_modules - 1;i >= 0;i--)
	{
		if (i == 0)
		{
			// Cannot unload pspnet_inet.prx currently;
			printk("%s: cannot unload pspnet_inet.prx currnetly\n", __func__);
			continue;
		}

		if (inet_modids[i] < 0)
		{
			printk("%s: %s was not loaded\n", __func__, inet_modules[i]);
			continue;
		}
		int result = 0;
		int stop_status = sceKernelStopModule(inet_modids[i], 0, NULL, &result, NULL);
		if (stop_status < 0)
		{
			printk("%s: failed stopping %s, 0x%x\n", __func__, stop_status);
		}
		int unload_status = sceKernelUnloadModule(inet_modids[i]);
		if (unload_status < 0)
		{
			printk("%s: failed unloading %s, 0x%x\n", __func__, inet_modules[i], unload_status);
			continue;
		}
		printk("%s: stopped and unloaded %s\n", __func__, inet_modules[i]);
		inet_modids[i] = -1;
	}
}
