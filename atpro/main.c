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

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspinit.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <psploadcore.h>
#include <psputilsforkernel.h>
#include <pspsysmem_kernel.h>
#include <pspctrl.h>
#include <psppower.h>
#include <pspwlan.h>
#include <psputility.h>
#include <psputility_netconf.h>
#include <psputility_sysparam.h>
#include <string.h>
#include <stdio.h>
#include "libs.h"
#include "hud.h"
#include "logs.h"
#include "systemctrl.h"

PSP_MODULE_INFO("ATPRO", PSP_MODULE_KERNEL, 1, 0);

// Game Code Getter (discovered in utility.prx)
const char * SysMemGameCodeGetter(void);

// System Control Module Patcher
STMOD_HANDLER sysctrl_patcher = NULL;

// Display Canvas
CANVAS displayCanvas;

// Input Thread Running Flag
int running = 0;

// HUD Overlay Flag
int hud_on = 0;

// Render-Wait State Flag
int wait = 0;

// Frame Counter
int framecount = 0;

// Online Mode Switch
int onlinemode = 0;

// sceKernelLoadModule Stub for 1.X FW
void * loadmodulestub = NULL;

// sceIoOpen Stub for 1.X FW
void * ioopenstub = NULL;

// sceKernelLoadModuleByID Stub for 1.X FW
void * loadmoduleiostub = NULL;

// sceIoClose Stub for 1.X FW
void * ioclosestub = NULL;

static const uint64_t LOAD_RETURN_MEMORY_THRES_USEC = 5000000;
static uint64_t game_begin = 0;

static SceUID stolen_memory = -1;

// Adhoc Module Names
#define MODULE_LIST_SIZE 5
char * module_names[MODULE_LIST_SIZE] = {
	"memab.prx",
	"pspnet_adhoc_auth.prx",
	"pspnet_adhoc.prx",
	"pspnet_adhocctl.prx",
	"pspnet_adhoc_matching.prx",
//	"pspnet_ap_dialog_dummy.prx",
//	"pspnet_adhoc_download.prx",
//	"pspnet_adhoc_discover.prx"
};

// Adhoc Module Dummy IO SceUIDs
SceUID module_io_uids[MODULE_LIST_SIZE] = {
	-1,
	-1,
	-1,
	-1,
	-1,
//	-1,
//	-1,
//	-1
};

SceUID shim_uid = -1;
static const char *shim_path = "ms0:/kd/pspnet_shims.prx";

static void *allocate_partition_memory(int size){
	SceUID uid = sceKernelAllocPartitionMemory(2, "inet apctl load reserve", 3 /* low aligned */, size, (void *)4);

	if (uid < 0)
	{
		printk("%s: failed allocating %d low aligned, 0x%x\n", __func__, size, uid);
		return NULL;
	}

	return sceKernelGetBlockHeadAddr(uid);
}

void steal_memory()
{
	static const int size = 1024 * 1024 * 4;

	if (stolen_memory >= 0)
	{
		printk("%s: refuse to steal memory again\n", __func__);
		return;
	}

	// https://github.com/Freakler/CheatDeviceRemastered/blob/fb0b45a254c724a2cef2397237b2d9ada22b37b4/source/utils.c
	SceUID test_alloc = sceKernelAllocPartitionMemory(2, "highmem probe", PSP_SMEM_High, 1024, NULL);

	if (test_alloc < 0)
	{
		printk("%s: cannot probe for memory layout, giving up\n", __func__);
		return;
	}

	void *test_head = sceKernelGetBlockHeadAddr(test_alloc);
	sceKernelFreePartitionMemory(test_alloc);

	if (test_head < 0x0B000000)
	{
		printk("%s: Not in high memory layout, 0x%x, releasing %d\n", __func__, test_head, test_alloc);
		return;
	}

	stolen_memory = sceKernelAllocPartitionMemory(2, "inet apctl load reserve", PSP_SMEM_Low, size, NULL);
	if (stolen_memory >= 0)
	{
		printk("%s: stole %d, id %d, head 0x%x\n", __func__, size, stolen_memory, sceKernelGetBlockHeadAddr(stolen_memory));
	}
	else
	{
		printk("%s: failed to steal memory, 0x%x\n", __func__, stolen_memory);
	}
}

void return_memory()
{
	if (stolen_memory < 0)
	{
		return;
	}

	sceKernelFreePartitionMemory(stolen_memory);
	printk("%s: returned memory\n", __func__);
	stolen_memory = -1;
}

char tolower(char value)
{
	if (value < 'A' || value > 'Z')
	{
		return value;
	}

	static const char distance = 'a' - 'A';
	return value + distance;
}

int strncasecmp(const char *lhs, const char *rhs, unsigned int max_len)
{
	int lhs_len = strnlen(lhs, max_len);
	int rhs_len = strnlen(rhs, max_len);

	if (lhs_len > 128 || rhs_len > 128)
	{
		printk("%s: simple strncasecmp implementation, giving up on strings that are too long\n", __func__);
		return 0;
	}
	if (lhs_len > rhs_len)
	{
		return 1;
	}
	if (rhs_len > lhs_len)
	{
		return -1;
	}

	char lhs_buf[128];
	char rhs_buf[128];
	for (int i = 0;i < lhs_len;i++)
	{
		lhs_buf[i] = tolower(lhs[i]);
	}
	for (int i = 0;i < rhs_len;i++)
	{
		rhs_buf[i] = tolower(rhs[i]);
	}
	return strncmp(lhs_buf, rhs_buf, max_len);
}

static int load_start_module(const char *path){
	uint32_t k1 = pspSdkSetK1(0);
	int uid = sceKernelLoadModule(path, 0, NULL);
	pspSdkSetK1(k1);
	if (uid < 0){
		printk("%s: failed loading %s, 0x%x\n", __func__, path, uid);
		return uid;
	}
	#ifdef DEBUG
	SceKernelModuleInfo info = {0};
	info.size = sizeof(info);
	k1 = pspSdkSetK1(0);
	int query_status = sceKernelQueryModuleInfo(uid, &info);
	pspSdkSetK1(k1);
	if (query_status == 0)
	{
		printk("%s: %s loaded, text addr 0x%x\n", __func__, path, info.text_addr);
	}
	else
	{
		printk("%s: failed fetching module info of %s, 0x%x\n", __func__, path, query_status);
	}
	#endif
	int module_start_ret;
	k1 = pspSdkSetK1(0);
	int start_status = sceKernelStartModule(uid, 0, NULL, &module_start_ret, NULL);
	pspSdkSetK1(k1);
	if (start_status < 0){
		printk("%s: failed starting %s, 0x%x\n", __func__, path, start_status);
		sceKernelUnloadModule(uid);
		return start_status;
	}
	return uid;
}

static const char *no_unload_modules[] = {
	"sceNet_Service",
	"sceNet_Library",
	"sceNetInet_Library",
	"sceNetApctl_Library",
	//"sceNetResolver_Library"
	"sceNetAdhoc_Library",
	"sceNetAdhocctl_Library",
	"sceNetAdhocMatching_Library",
	"sceNetAdhocAuth_Service",
	"sceMemab"
};

static const char *no_unload_module_file_names[] = {
	"ifhandle.prx",
	"pspnet.prx",
	"pspnet_inet.prx",
	"pspnet_apctl.prx",
	//"pspnet_resolver.prx",
	"pspnet_adhoc.prx",
	"pspnet_adhocctl.prx",
	"pspnet_adhoc_matching.prx",
	"pspnet_adhoc_auth.prx",
	"memab.prx"
};

static SceUID no_unload_module_uids[] = {
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

static int no_unload_module_started[] = {
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

// Kernel Module Loader
typedef SceUID (*module_loader_func)(const char * path, int flags, SceKernelLMOption * option);
module_loader_func load_plugin_user_orig = NULL;
void load_module_loader_functions()
{
	if (load_plugin_user_orig == NULL)
	{
		load_plugin_user_orig = (module_loader_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0x977DE386);
	}
}
SceUID load_plugin(const char * path, int flags, SceKernelLMOption * option, module_loader_func orig);
SceUID load_plugin_kernel(const char * path, int flags, SceKernelLMOption * option)
{
	if (option == NULL)
	{
		printk("%s: loading %s without options\n", __func__, path);
	}
	else
	{
		printk("%s: loading %s into partition %d/%d with position %d\n", __func__, path, option->mpidtext, option->mpiddata, option->position);
	}
	load_module_loader_functions();
	return load_plugin(path, flags, option, sceKernelLoadModule);
}
SceUID load_plugin_user(const char * path, int flags, SceKernelLMOption * option)
{
	if (option == NULL)
	{
		printk("%s: loading %s without options\n", __func__, path);
	}
	else
	{
		printk("%s: loading %s into partition %d/%d with position %d\n", __func__, path, option->mpidtext, option->mpiddata, option->position);
	}

	load_module_loader_functions();
	return load_plugin(path, flags, option, load_plugin_user_orig);
}
SceUID load_plugin(const char * path, int flags, SceKernelLMOption * option, module_loader_func orig)
{
	// Online Mode Enabled
	if(onlinemode)
	{
		// Force module path case
		char test_path[256] = {0};

		int len = strlen(path);
		for(int i = 0;i < len;i++){
			test_path[i] = tolower(path[i]);
		}

		static const char *force_fw_modules[] = {
			"ifhandle.prx",
			"pspnet.prx",
			"pspnet_inet.prx",
			"pspnet_apctl.prx",
			"pspnet_resolver.prx",
			"sc_sascore.prx"
		};

		for (int i = 0;i < sizeof(force_fw_modules) / sizeof(char *) && onlinemode;i++)
		{
			if (strstr(test_path, force_fw_modules[i]) != NULL && strstr(test_path, "disc0:/") != NULL)
			{
				printk("%s: forcing firmware %s\n", __func__, force_fw_modules[i]);
				sprintf(path, "flash0:/kd/%s", force_fw_modules[i]);
				break;
			}
		}

		// If these were already loaded prior
		for(int i = 0;i < sizeof(no_unload_module_file_names) / sizeof(char *);i++){
			if (strstr(test_path, no_unload_module_file_names[i]))
			{
				if (no_unload_module_uids[i] >= 0)
				{
					printk("%s: returning previous uid 0x%x for %s\n", __func__, no_unload_module_uids[i], path);
					return no_unload_module_uids[i];
				}
			}
		}

		// Games might also load inet itself for infra mode
		static const char *late_load_modules[] = {
			"ifhandle.prx",
			"pspnet.prx",
			"pspnet_inet.prx",
			"pspnet_apctl.prx",
			"pspnet_resolver.prx"
		};

		if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC){
			for (int i = 0;i < sizeof(late_load_modules) / sizeof(char *);i++)
			{
				if (strstr(test_path, late_load_modules[i]) != NULL)
				{
					return_memory();
					break;
				}
			}
		}

		// Replace Adhoc Modules
		int i = 0; for(; i < MODULE_LIST_SIZE; i++) {
			// Matching Modulename
			if(strstr(test_path, module_names[i]) != NULL) {
				// Replace Modulename
				strcpy((char*)path, "ms0:/kd/");
				strcpy((char*)path + strlen(path), module_names[i]);
				
				if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
				{
					return_memory();
				}

				// Fix Permission Error
				uint32_t k1 = pspSdkSetK1(0);

				// Load Module
				SceUID result = sceKernelLoadModule(path, flags, option);

				// Restore K1 Register
				pspSdkSetK1(k1);

				if (result < 0)
				{
					printk("%s: failed loading %s, 0x%x\n", __func__, module_names[i], result);
					steal_memory();
				}
				
				// Log Hotswapping
				printk("%s: Swapping %s, UID=0x%08X\n", __func__, module_names[i], result);

				for(int i = 0;i < sizeof(no_unload_module_file_names) / sizeof(char *);i++){
					if (strstr(test_path, no_unload_module_file_names[i]) != NULL)
					{
						no_unload_module_uids[i] = result;
					}
				}

				// Return Module UID
				return result;
			}
		}

		// Load shim if needed
		static const char *shimmed_modules[] = {
			"pspnet_apctl.prx"
		};

		for(int i = 0;i < sizeof(shimmed_modules) / sizeof(char *);i++)
		{
			if (strstr(test_path, shimmed_modules[i]) != NULL && shim_uid < 0)
			{
				if (shim_uid < 0)
				{
					shim_uid = load_start_module(shim_path);
				}
			}
		}
	}

	// Default Action - Load Module

	int result = orig(path, flags, option);

	#if 1
	// might be PSVita, or forcing firmware version of modules
	if (result < 0)
	{
		printk("%s: module load failed with current k1, 0x%x, trying again with kernel k1\n", __func__, result);

		uint32_t k1 = pspSdkSetK1(0);
		result = sceKernelLoadModule(path, flags, option);
		pspSdkSetK1(k1);
	}
	#endif

	if (result < 0)
	{
		printk("%s: failed loading %s, 0x%x\n", __func__, path, result);
	}

	// Since we replaced stargate's module load hook (I hope), do this here
	// https://github.com/MrColdbird/procfw/blob/master/Stargate/loadmodule_patch.c
	// https://github.com/PSP-Archive/ARK-4/blob/main/core/stargate/loadmodule_patch.c
	// XXX do we still need this in 660+ ?
	if (result == 0x80020148 || result == 0x80020130) {
		if (!strncasecmp(path, "ms0:", sizeof("ms0:")-1)) {
			result = 0x80020146;
			printk("%s: [FAKE] -> 0x%08X\n", __func__, result);
		}
	}

	for(int i = 0;i < sizeof(no_unload_module_file_names) / sizeof(char *);i++){
		if (strstr(path, no_unload_module_file_names[i]))
		{
			no_unload_module_uids[i] = result;
		}
	}

	return result;
}

typedef int (*module_unload_func)(SceUID uid);
int unload_plugin(SceUID uid, module_unload_func);
int unload_plugin_kernel(SceUID uid){
	return unload_plugin(uid, sceKernelUnloadModule);
}
module_unload_func unload_plugin_user_orig = NULL;
int unload_plugin_user(SceUID uid)
{
	if (unload_plugin_user_orig == NULL){
		unload_plugin_user_orig = (module_unload_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0x2E0911AA);
	}
	return unload_plugin(uid, unload_plugin_user_orig);
}
int unload_plugin(SceUID uid, module_unload_func orig)
{
	// Fetch module info
	SceKernelModuleInfo info = {0};
	info.size = sizeof(info);

	uint32_t k1 = pspSdkSetK1(0);
	int query_status = sceKernelQueryModuleInfo(uid, &info);
	pspSdkSetK1(k1);
	if (query_status != 0){
		printk("%s: failed fetching module name\n", __func__);
		return orig(uid);
	}

	printk("%s: unloading %s\n", __func__, info.name);

	for(int i = 0;i < sizeof(no_unload_modules) / sizeof(char *) && onlinemode;i++){
		// Stop these modules from being unloaded
		if (strstr(info.name, no_unload_modules[i]) != NULL){
			printk("%s: blocked %s unloading\n", __func__, no_unload_modules[i]);
			return 0;
		}
	}

	return orig(uid);
}

typedef int (*module_start_func)(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option);
int start_plugin(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option, module_start_func);
int start_plugin_kernel(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option){
	return start_plugin(uid, argsize, argp, status, option, sceKernelStartModule);
}
module_start_func start_plugin_user_orig = NULL;
int start_plugin_user(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	if (start_plugin_user_orig == NULL){
		start_plugin_user_orig = (module_start_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0x50F0C1EC);
	}
	return start_plugin(uid, argsize, argp, status, option, start_plugin_user_orig);
}
int start_plugin(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option, module_start_func orig)
{
	// Fetch module info
	SceKernelModuleInfo info = {0};
	info.size = sizeof(info);

	uint32_t k1 = pspSdkSetK1(0);
	int query_status = sceKernelQueryModuleInfo(uid, &info);
	pspSdkSetK1(k1);
	if (query_status != 0){
		printk("%s: failed fetching module name\n", __func__);
		return orig(uid, argsize, argp, status, option);
	}

	printk("%s: starting %s\n", __func__, info.name);

	for(int i = 0;i < sizeof(no_unload_modules) / sizeof(char *) && onlinemode;i++){
		// Start these modules once
		if (strstr(info.name, no_unload_modules[i]) != NULL){
			if (no_unload_module_started[i] < 0)
			{
				no_unload_module_started[i] = orig(uid, argsize, argp, status, option);
				printk("%s: %s marked as 0x%x\n", __func__, no_unload_modules[i], no_unload_module_started[i]);
			}
			else
			{
				printk("%s: not starting %s again\n", __func__, no_unload_modules[i]);
			}
			return no_unload_module_started[i];
		}
	}

	return orig(uid, argsize, argp, status, option);
}

typedef int (*module_stop_func)(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option);
int stop_plugin(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option, module_stop_func);
int stop_plugin_kernel(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option){
	return stop_plugin(uid, argsize, argp, status, option, sceKernelStopModule);
}
module_stop_func stop_plugin_user_orig = NULL;
int stop_plugin_user(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	if (stop_plugin_user_orig == NULL){
		stop_plugin_user_orig = (module_stop_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0xD1FF982A);
	}
	return stop_plugin(uid, argsize, argp, status, option, stop_plugin_user_orig);
}
int stop_plugin(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option, module_stop_func orig)
{
	// Fetch module info
	SceKernelModuleInfo info = {0};
	info.size = sizeof(info);

	uint32_t k1 = pspSdkSetK1(0);
	int query_status = sceKernelQueryModuleInfo(uid, &info);
	pspSdkSetK1(k1);
	if (query_status != 0){
		printk("%s: failed fetching module name\n", __func__);
		return orig(uid, argsize, argp, status, option);
	}

	printk("%s: stopping %s\n", __func__, info.name);

	for(int i = 0;i < sizeof(no_unload_modules) / sizeof(char *) && onlinemode;i++){
		// Do not stop these modules
		if (strstr(info.name, no_unload_modules[i]) != NULL){
			printk("%s: blocked stopping of %s\n", __func__, no_unload_modules[i]);
			return 0;
		}
	}

	return orig(uid, argsize, argp, status, option);
}

// User Module Loader
SceUID load_plugin_alt(const char * path, int unk1, int unk2, int flags, SceKernelLMOption * option)
{
	// Thanks to CFW we can load whatever we want...
	// No need to have different loader for user & kernel modules.
	return load_plugin_kernel(path, flags, option);
}

#if 0
// IO Plugin File Loader
SceUID load_plugin_io(SceUID fd, int flags, SceKernelLMOption * option)
{
	// Online Mode Enabled
	if(onlinemode)
	{
		// Replace Adhoc Modules
		int i = 0; for(; i < MODULE_LIST_SIZE; i++) {
			// Matching Fake UID
			if(module_io_uids[i] == fd) {
				// Create Module Path
				char path[256];
				strcpy(path, "ms0:/kd/");
				strcpy(path + strlen(path), module_names[i]);

				if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
				{
					return_memory();
				}
				
				// Avoid 0x80020149 Illegal Permission Error
				uint32_t k1 = pspSdkSetK1(0);
				
				// Load Module
				SceUID result = sceKernelLoadModule(path, flags, option);
				
				// Restore K1 Register
				pspSdkSetK1(k1);

				if (result < 0)
				{
					printk("%s: failed loading %s, 0x%x\n", __func__, module_names[i], result);
					steal_memory();
				}

				// Log Hotswapping
				printk("%s: Swapping %s, UID=0x%08X\n", __func__, module_names[i], result);
				
				// Return Module UID
				return result;
			}
		}
	}
	
	// Find Function
	int (* originalcall)(SceUID, int, SceKernelLMOption *) = (void *)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0xB7F46618);
	
	// Default Action - Load Module

	int result = originalcall(fd, flags, option);

	// might be PSVita
	if (result < 0)
	{
		printk("%s: module load failed with current k1, 0x%x, trying again with kernel k1\n", __func__, result);

		uint32_t k1 = pspSdkSetK1(0);
		result = originalcall(fd, flags, option);
		pspSdkSetK1(k1);
	}

	if (result < 0)
	{
		printk("%s: failed loading from id %d, 0x%x\n", __func__, fd, result);
	}

	return result;
}

// Plugin File Loader
SceUID open_plugin(char * path, int flags, int mode)
{
	// Online Mode Enabled
	if(onlinemode)
	{
		// Compare Adhoc Module Names
		int i = 0; for(; i < MODULE_LIST_SIZE; i++) {
			// Matching Modulename
			if(strstr(path, module_names[i]) != NULL) {
				// Override File Path
				strcpy(path, "ms0:/kd/");
				strcpy(path + strlen(path), module_names[i]);

				if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
				{
					return_memory();
				}

				// Open File
				SceUID result = sceIoOpen(path, flags, mode);

				if (result < 0)
				{
					printk("%s: failed opening %s, 0x%x\n", __func__, path, result);
					steal_memory();
				}
				
				// Valid Result
				if(result >= 0)
				{
					// Save UID
					module_io_uids[i] = result;
				}
				
				// Log File Open
				printk("%s: Opening %s File Handle, UID=0x%08X\n", __func__, module_names[i], result);
				
				// Return File UID
				return result;
			}
		}
	}
	
	// Default Action - Open File
	int result = sceIoOpen(path, flags, mode);
	printk("%s: opened %s as %d\n", __func__, path, result);
	return result;
}

// Plugin File Closer
int close_plugin(SceUID fd)
{
	// Online Mode Enabled
	if(onlinemode)
	{
		// Replace Adhoc Modules
		int i = 0; for(; i < MODULE_LIST_SIZE; i++) {
			// Matching IO UID
			if(module_io_uids[i] == fd) {
				// Log Close
				printk("Closing %s File Handle, UID=0x%08X\n", module_names[i], module_io_uids[i]);
				
				// Erase UID
				module_io_uids[i] = -1;
				
				// Stop Searching
				break;
			}
		}
	}
	
	// Default Action - Close File
	return sceIoClose(fd);
}
#endif

// Game Code Getter
const char * getGameCode(void)
{
	// 620 SysMemForKernel_AB5E85E5
	// 63X SysMemForKernel_3C4C5630
	// 660 SysMemForKernel_EF29061C
	return SysMemGameCodeGetter() + 0x44;
}

// Callback Deny Function
int cbdeny(int cbid)
{
	// By doing nothing, we prevent exit callback registration, thus block the home menu.
	return 0;
}

// Killzone Fixed Pool Size Limiter
int killzone_createfpl(char * name, int pid, uint32_t attr, uint32_t size, int blocks, void * param)
{
	// Killzone DummyHeap (Allocates memory and frees again to test how much it can allocate without error...)
	if(strcmp(name, "DummyHeap") == 0)
	{
		// Limit Maximum Size
		if(size > 0x1800000) return 0x80020190; // Insufficient Memory
	}
	
	// Allocate Memory
	return sceKernelCreateFpl(name, pid, attr, size, blocks, param);
}

int get_system_param_int(int id, int *value)
{
	if (id == PSP_SYSTEMPARAM_ID_INT_ADHOC_CHANNEL)
	{
		*value = 1;
		return 0;
	}

	return sceUtilityGetSystemParamInt(id, value);
}


pspUtilityNetconfData *netconf_override;
struct pspUtilityNetconfAdhoc *netconf_adhoc_override;
int (*netconf_init_orig)(pspUtilityNetconfData *data) = NULL;
int netconf_init(pspUtilityNetconfData *data){
	if (data != NULL)
	{
		printk("%s: data size is %d, expected %d\n", __func__, data->base.size, sizeof(pspUtilityNetconfData));
	}

	if (data != NULL && data->action == PSP_NETCONF_ACTION_CONNECTAP && netconf_override != NULL && netconf_adhoc_override != NULL)
	{
		printk("%s: overriding netconf param for infra mode\n", __func__);
		int ctrl = 1;
		int lang = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
		sceUtilityGetSystemParamInt(9, &ctrl);
		sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &lang);

		data = netconf_override;
		memset(netconf_override, 0, sizeof(pspUtilityNetconfData));
		netconf_override->base.size = sizeof(pspUtilityNetconfData);
		netconf_override->base.language = lang;
		netconf_override->base.buttonSwap = ctrl;
		netconf_override->base.graphicsThread = 17;
		netconf_override->base.accessThread = 19;
		netconf_override->base.fontThread = 18;
		netconf_override->base.soundThread = 16;

		netconf_override->action = PSP_NETCONF_ACTION_CONNECTAP;
		netconf_override->adhocparam = netconf_adhoc_override;
		memset(netconf_adhoc_override, 0, sizeof(struct pspUtilityNetconfAdhoc));
	}

	int result = netconf_init_orig(data);
	printk("%s: returning 0x%x/%d\n", __func__, result, result);

	return result;
}

// Patcher to allow Utility-Made Connections
void patch_netconf_utility(void * init, void * getstatus, void * update, void * shutdown)
{
	// Module ID Buffer
	static int id[100];
	
	// Number of Modules
	int idcount = 0;
	
	// Find Module IDs
	uint32_t k1 = pspSdkSetK1(0);
	int result = sceKernelGetModuleIdList(id, sizeof(id), &idcount);
	pspSdkSetK1(k1);
	
	// Found Module IDs
	if(result == 0)
	{
		// Iterate Modules
		int i = 0; for(; i < idcount; i++)
		{
			// Find Module
			SceModule2 * module = (SceModule2 *)sceKernelFindModuleByUID(id[i]);

			if (strcmp(module->modname, "sceNetAdhocctl_Library") == 0){
				printk("%s: avoid patching netconf of sceNetAdhocctl_Library\n", __func__);
				continue;
			}

			// Found Userspace Module
			if(module != NULL && (module->text_addr & 0x80000000) == 0)
			{
				// Hook Imports
				hook_import_bynid((SceModule *)module, "sceUtility", 0x4DB1E739, init);
				hook_import_bynid((SceModule *)module, "sceUtility", 0x6332AA39, getstatus);
				hook_import_bynid((SceModule *)module, "sceUtility", 0x91E70E35, update);
				hook_import_bynid((SceModule *)module, "sceUtility", 0xF88155F6, shutdown);
			}
		}
	}
}

// Framebuffer Setter
int setframebuf(void *topaddr, int bufferwidth, int pixelformat, int sync)
{
	// Increase Frame Counter
	framecount++;
	
	// Ready to Paint State
	if(wait == 0)
	{
		// Lock State
		wait = 1;
		
		// Get Canvas Information
		int mode = 0; sceDisplayGetMode(&mode, &(displayCanvas.width), &(displayCanvas.height));
		
		// Update Canvas Information
		displayCanvas.buffer = topaddr;
		displayCanvas.lineWidth = bufferwidth;
		displayCanvas.pixelFormat = pixelformat;
		
		// HUD Painting Required
		if(hud_on) drawInfo(&displayCanvas);
		
		// Notification Painting Required
		else drawNotification(&displayCanvas);
		
		// Unlock State
		wait = 0;
	}
	
	// Passthrough
	return sceDisplaySetFrameBuf(topaddr, bufferwidth, pixelformat, sync);
}

// Read Positive Null & Passthrough Hook
int read_buffer_positive(SceCtrlData * pad_data, int count)
{
	// Passthrough
	int result = sceCtrlReadBufferPositive(pad_data, count);
	
	// PRO HUD on screen
	if(hud_on)
	{
		// Iterate Elements
		int i = 0; for(; i < count; i++)
		{
			// Erase Digital Buttons
			pad_data[i].Buttons = 0;
		}
	}
	
	// Return Result
	return result;
}

// Peek Positive Null & Passthrough Hook
int peek_buffer_positive(SceCtrlData * pad_data, int count)
{
	// Passthrough
	int result = sceCtrlPeekBufferPositive(pad_data, count);
	
	// PRO HUD on screen
	if(hud_on)
	{
		// Iterate Elements
		int i = 0; for(; i < count; i++)
		{
			// Erase Digital Buttons
			pad_data[i].Buttons = 0;
		}
		return 0;
	}
	
	// Return Result
	return result;
}

// Read Negative Null & Passthrough Hook
int read_buffer_negative(SceCtrlData * pad_data, int count)
{
	// Passthrough
	int result = sceCtrlReadBufferNegative(pad_data, count);
	
	// PRO HUD on screen
	if(hud_on)
	{
		// Iterate Elements
		int i = 0; for(; i < count; i++)
		{
			// Erase Digital Buttons
			pad_data[i].Buttons = 0;
		}
		return 0;
	}
	
	// Return Result
	return result;
}

// Peek Negative Null & Passthrough Hook
int peek_buffer_negative(SceCtrlData * pad_data, int count)
{
	// Passthrough
	int result = sceCtrlPeekBufferNegative(pad_data, count);
	
	// PRO HUD on screen
	if(hud_on)
	{
		// Iterate Elements
		int i = 0; for(; i < count; i++)
		{
			// Erase Digital Buttons
			pad_data[i].Buttons = 0;
		}
		return 0;
	}
	
	// Return Result
	return result;
}

#if 0
// Create 1.X FW sceKernelLoadModule Stub
void * create_loadmodule_stub(void)
{
	// Find Allocator Functions in Memory
	int (* alloc)(u32, char *, u32, u32, u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x237DBD4F);
	void * (* gethead)(u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x9D9A5BA1);

	// Allocate Memory
	int result = alloc(2, "LoadModuleStub", PSP_SMEM_High, 8, 0);
	
	// Allocated Memory
	if(result >= 0)
	{
		// Get Memory Block
		uint32_t * asmblock = gethead(result);
		
		// Got Memory Block
		if(asmblock != NULL)
		{
			// Link to Syscall
			asmblock[0] = 0x03E00008; // jr $ra
			asmblock[1] = MAKE_SYSCALL(sctrlKernelQuerySystemCall(load_plugin));
			
			// Return sceKernelLoadModule Stub
			return (void *)asmblock;
		}
	}
	
	// Allocation Error
	return NULL;
}

// Create 1.X FW sceIoOpen Stub
void * create_ioopen_stub(void)
{
	// Find Allocator Functions in Memory
	int (* alloc)(u32, char *, u32, u32, u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x237DBD4F);
	void * (* gethead)(u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x9D9A5BA1);

	// Allocate Memory
	int result = alloc(2, "IOOpenStub", PSP_SMEM_High, 8, 0);
	
	// Allocated Memory
	if(result >= 0)
	{
		// Get Memory Block
		uint32_t * asmblock = gethead(result);
		
		// Got Memory Block
		if(asmblock != NULL)
		{
			// Link to Syscall
			asmblock[0] = 0x03E00008; // jr $ra
			asmblock[1] = MAKE_SYSCALL(sctrlKernelQuerySystemCall(open_plugin));
			
			// Return sceKernelLoadModule Stub
			return (void *)asmblock;
		}
	}
	
	// Allocation Error
	return NULL;
}

// Create 1.X FW sceKernelLoadModuleByID Stub
void * create_loadmoduleio_stub(void)
{
	// Find Allocator Functions in Memory
	int (* alloc)(u32, char *, u32, u32, u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x237DBD4F);
	void * (* gethead)(u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x9D9A5BA1);

	// Allocate Memory
	int result = alloc(2, "LoadModuleIOStub", PSP_SMEM_High, 8, 0);
	
	// Allocated Memory
	if(result >= 0)
	{
		// Get Memory Block
		uint32_t * asmblock = gethead(result);
		
		// Got Memory Block
		if(asmblock != NULL)
		{
			// Link to Syscall
			asmblock[0] = 0x03E00008; // jr $ra
			asmblock[1] = MAKE_SYSCALL(sctrlKernelQuerySystemCall(load_plugin_io));
			
			// Return sceKernelLoadModule Stub
			return (void *)asmblock;
		}
	}
	
	// Allocation Error
	return NULL;
}

// Create 1.X FW sceIoClose Stub
void * create_ioclose_stub(void)
{
	// Find Allocator Functions in Memory
	int (* alloc)(u32, char *, u32, u32, u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x237DBD4F);
	void * (* gethead)(u32) = (void *)sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x9D9A5BA1);

	// Allocate Memory
	int result = alloc(2, "IOCloseStub", PSP_SMEM_High, 8, 0);
	
	// Allocated Memory
	if(result >= 0)
	{
		// Get Memory Block
		uint32_t * asmblock = gethead(result);
		
		// Got Memory Block
		if(asmblock != NULL)
		{
			// Link to Syscall
			asmblock[0] = 0x03E00008; // jr $ra
			asmblock[1] = MAKE_SYSCALL(sctrlKernelQuerySystemCall(close_plugin));
			
			// Return sceKernelLoadModule Stub
			return (void *)asmblock;
		}
	}
	
	// Allocation Error
	return NULL;
}
#endif

static void early_memory_stealing()
{
	static int stole_memory_here = 0;
	// Steal some memory from game in case it tries to allocate as much as it can on start
	if (!stole_memory_here)
	{
		steal_memory();
		if (stolen_memory >= 0)
		{
			stole_memory_here = 1;
		}
	}
	else
	{
		if (stolen_memory >= 0)
		{
			printk("%s: not stealing memory here again, stolen memory head 0x%x id %d\n", __func__, sceKernelGetBlockHeadAddr(stolen_memory), stolen_memory);
		}
		else
		{
			printk("%s: not stealing memory here again\n", __func__);
		}
	}
}

// Online Module Start Patcher
int online_patcher(SceModule2 * module)
{
	// Try to do this before stargate
	int sysctrl_patcher_result = sysctrl_patcher(module);

	printk("%s: module start %s text_addr 0x%x\n", __func__, module->modname, module->text_addr);

	if (module->text_addr > 0x08800000 && module->text_addr < 0x08900000 && strcmp("opnssmp", module->modname) != 0 && onlinemode)
	{
		// Very likely the game itself
		printk("%s: guessing this is the game, %s text_addr 0x%x, trying to reserve memory now and hook mdoule loading/unloading\n", __func__, module->modname, module->text_addr);
		early_memory_stealing();
		hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x977DE386, load_plugin_user);
		hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x2E0911AA, unload_plugin_user);
		hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x50F0C1EC, start_plugin_user);
		hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0xD1FF982A, stop_plugin_user);

		// allocate memory for netconf
		//netconf_override = allocate_partition_memory(sizeof(allocate_partition_memory));
		//netconf_adhoc_override = allocate_partition_memory(sizeof(struct pspUtilityNetconfAdhoc));
		netconf_override = allocate_partition_memory(128);
		netconf_adhoc_override = (void *)(((uint32_t)netconf_override) + 72);
	}

	// Userspace Module
	if((module->text_addr & 0x80000000) == 0)
	{
		if (game_begin == 0)
		{
			game_begin = sceKernelGetSystemTimeWide();
		}

		// Might be Untold Legends - Brotherhood of the Blade...
		if(strcmp(module->modname, "etest") == 0)
		{
			// Offsets
			uint32_t loader = 0;
			uint32_t unloader = 0;
			
			// European Version
			if(strcmp(getGameCode(), "ULES00046") == 0)
			{
				// Fill in Offsets
				loader = 0x73F40;
				unloader = 0x74334;
			}
			
			// US Version
			else if(strcmp(getGameCode(), "ULUS10003") == 0)
			{
				// Fill in Offsets
				loader = 0x6EB24;
				unloader = 0x6EF18;
			}
			
			// JPN Version doesn't need this fix. Sony fixed it themselves.
			
			// Valid Game Version
			if(loader != 0 && unloader != 0)
			{
				// Calculate Offsets
				loader += module->text_addr;
				unloader += module->text_addr;
				
				// Syscall Numbers
				uint32_t loadutility = sctrlKernelQuerySystemCall((void *)sctrlHENFindFunction("sceUtility_Driver", "sceUtility", 0x2A2B3DE0));
				uint32_t unloadutility = sctrlKernelQuerySystemCall((void *)sctrlHENFindFunction("sceUtility_Driver", "sceUtility", 0xE49BFE92));
				
				// Fix Module Loader
				// C-Summary:
				// sceUtilityLoadModule(PSP_MODULE_NET_COMMON);
				// sceUtilityLoadModule(PSP_MODULE_NET_ADHOC);
				// return;
				
				_sw(0x24040100, loader); // li $a0, 0x100 (arg1 = PSP_MODULE_NET_COMMON)
				_sw(MAKE_SYSCALL(loadutility), loader + 4); // sceUtilityLoadModule(PSP_MODULE_NET_COMMON);
				_sw(0x24040101, loader + 8); // li $a0, 0x101 (arg1 = PSP_MODULE_NET_ADHOC)
				_sw(0x03E00008, loader + 12); // jr $ra
				_sw(MAKE_SYSCALL(loadutility), loader + 16); // sceUtilityLoadModule(PSP_MODULE_NET_ADHOC);
				
				// Fix Module Unloader
				// C-Summary:
				// sceUtilityUnloadModule(PSP_MODULE_NET_COMMON);
				// sceUtilityUnloadModule(PSP_MODULE_NET_ADHOC);
				// return;
				
				_sw(0x24040101, unloader); // li $a0, 0x101 (arg1 = PSP_MODULE_NET_ADHOC)
				_sw(MAKE_SYSCALL(unloadutility), unloader + 4); // sceUtilityUnloadModule(PSP_MODULE_NET_ADHOC);
				_sw(0x24040100, unloader + 8); // li $a0, 0x100 (arg1 = PSP_MODULE_NET_COMMON)
				_sw(0x03E00008, unloader + 12); // jr $ra
				_sw(MAKE_SYSCALL(unloadutility), unloader + 16); // sceUtilityUnloadModule(PSP_MODULE_NET_COMMON);
				
				// Invalidate Caches
				sceKernelDcacheWritebackInvalidateRange((void *)loader, 20);
				sceKernelIcacheInvalidateRange((void *)loader, 20);
				sceKernelDcacheWritebackInvalidateRange((void *)unloader, 20);
				sceKernelIcacheInvalidateRange((void *)unloader, 20);
				
				// Log Game-Specific Patch
				printk("Patched %s with updated Module Loader\n", getGameCode());
			}
		}
		
		// Might be Killzone - Liberation...
		else if(strcmp(module->modname, "Guerrilla") == 0)
		{
			// US / EU / KR Version
			if(strcmp(getGameCode(), "UCUS98646") == 0 || strcmp(getGameCode(), "UCES00279") == 0 || strcmp(getGameCode(), "UCKS45041") == 0)
			{
				// Install Memory Allocation Limiter
				hook_import_bynid((SceModule *)module, "ThreadManForUser", 0xC07BB470, killzone_createfpl);
				
				// Log Game-Specific Patch
				printk("Patched %s with Fixed Pool Size Limiter\n", getGameCode());
			}
		}
		
		// Generic 1.X Game User Module Fixer
		else if(strstr(module->modname, "Adhoc") == NULL)
		{
			/*
			// sceKernelLoadModule Stub not yet created
			if(loadmodulestub == NULL)
			{
				// Create sceKernelLoadModule Stub
				loadmodulestub = create_loadmodule_stub();
				ioopenstub = create_ioopen_stub();
				loadmoduleiostub = create_loadmoduleio_stub();
				ioclosestub = create_ioclose_stub();
			}
			*/
			
			// sceKernelLoadModule Stub available
			// if(loadmodulestub != NULL)
			{
				// hook_weak_user_bynid is more permanent than hook_import_bynid as it can't be undone by the module manager
				// so... more games can be affected by it... however it causes a lot of games to glitch... :(
				
				// Hook sceKernelLoadModule
				// hook_weak_user_bynid(module, "ModuleMgrForUser", 0x977DE386, loadmodulestub);
				
				// Let stargate hide cfw files keep the rest
				#if 0
				// Hook sceIoOpen
				// hook_weak_user_bynid(module, "IoFileMgrForUser", 0x109F50BC, ioopenstub);
				hook_import_bynid((SceModule *)module, "IoFileMgrForUser", 0x109F50BC, open_plugin);
				
				// Hook sceKernelLoadModuleByID
				// hook_weak_user_bynid(module, "ModuleMgrForUser", 0xB7F46618, loadmoduleiostub);
				hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0xB7F46618, load_plugin_io);
				
				// Hook sceIoClose
				// hook_weak_user_bynid(module, "IoFileMgrForUser", 0x810C4BC3, ioclosestub);
				hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x810C4BC3, close_plugin);

				// Log Patch
				printk("Patched %s with sceKernelLoadModule Hook\n", module->modname);
				#endif
			}
		}

		// Hook shims
		if (onlinemode)
		{
			if (strstr(module->modname, "sceNetApctl_Library")){
				void (*hijack_sceNetApctlInit)() = (void (*)())sctrlHENFindFunction("pspnet_shims", "pspnet_shims", 0x1);
				if (hijack_sceNetApctlInit != NULL)
				{
					printk("%s: redirecting sceNetApctlInit\n", __func__);
					hijack_sceNetApctlInit();					
				}else{
					printk("%s: sceNetApctlInitShim is null!\n", __func__);
				}
			}

			// Lie to the game about adhoc channel for at least Ridge Racer 2
			hook_import_bynid((SceModule *)module, "sceUtility", 0xA5DA2406, get_system_param_int);
		}

		// Hook Framebuffer Setter
		hook_import_bynid((SceModule *)module, "sceDisplay", 0x289D82FE, setframebuf);
		
		// Hook Controller Input (for Game Isolation)
		hook_import_bynid((SceModule *)module, "sceCtrl", 0x1F803938, read_buffer_positive);
		hook_import_bynid((SceModule *)module, "sceCtrl", 0x3A622550, peek_buffer_positive);
		hook_import_bynid((SceModule *)module, "sceCtrl", 0x60B81F86, read_buffer_negative);
		hook_import_bynid((SceModule *)module, "sceCtrl", 0xC152080A, peek_buffer_negative);
	}
	
	// Enable System Control Patching
	return sysctrl_patcher_result;
}

// Input Thread
int input_thread(SceSize args, void * argp)
{
	// Previous Buttons
	uint32_t prev_buttons = 0;
	
	// Current Buttons
	uint32_t curr_buttons = 0;
	
	// Kernel Clock Variables
	SceKernelSysClock clock_start, clock_end;
	
	// Exit Button pressed
	int is_exit_button_pressed = 0;
	
	// Exit Button Clock Variables
	SceKernelSysClock exit_press_start, exit_press_end;
	
	// Endless Loop
	while(running == 1)
	{
		// Init Logic Timer
		sceKernelGetSystemTime(&clock_start);
		
		// Move Buttons
		prev_buttons = curr_buttons;
		
		// Button Data Holder
		SceCtrlData ctrl;
		
		// Read Buttons
		sceCtrlPeekBufferPositive(&ctrl, 1);
		
		// Register Button
		curr_buttons = ctrl.Buttons;
		
		// Home Button pressed (and not pressing exit button)
		if(!is_exit_button_pressed && (prev_buttons & PSP_CTRL_HOME) == 0 && (curr_buttons & PSP_CTRL_HOME) != 0)
		{
			// Enable or Disable GUI Overlay
			hud_on = !hud_on;
		}
		
		// Home Menu Button Events
		else if(hud_on)
		{
			// First Exit Button Press
			if(!is_exit_button_pressed && (prev_buttons & PSP_CTRL_START) == 0 && (curr_buttons & PSP_CTRL_START) != 0)
			{
				// Activate Exit Button Press Flag
				is_exit_button_pressed = 1;
				
				// Start Press Timer
				sceKernelGetSystemTime(&exit_press_start);
				
				// Clone Data to End Press Timer
				exit_press_end = exit_press_start;
			}
			
			// Lifted Exit Button
			else if(is_exit_button_pressed && (prev_buttons & PSP_CTRL_START) != 0 && (curr_buttons & PSP_CTRL_START) == 0)
			{
				// Deactivate Exit Button Press Flag
				is_exit_button_pressed = 0;
			}
			
			// Held Exit Button
			else if(is_exit_button_pressed && (prev_buttons & PSP_CTRL_START) != 0 && (curr_buttons & PSP_CTRL_START) != 0)
			{
				// Update End Press Timer
				sceKernelGetSystemTime(&exit_press_end);
			}
			
			// Exit to VSH (if button got pressed for 3 seconds)
			if(is_exit_button_pressed && (exit_press_end.low - exit_press_start.low) >= 3000000)
			{
				// Reboot into VSH
				sceKernelExitVSHVSH(NULL);
				
				// Stop Processing
				break;
			}
			
			// Pass Event to HUD Handler
			else handleKeyEvent(prev_buttons, curr_buttons);
		}
		
		// No-Wait State
		if(wait == 0)
		{
			// Block Drawing Operation
			wait = 1;
			
			// Update Canvas
			if(getCanvas(&displayCanvas) == 0)
			{
				// Wait for V-Blank
				sceDisplayWaitVblankStart();
				
				// Calculate Vertical Blank Times
				int vblank = (int)(1000000.0f / sceDisplayGetFramePerSec());
				int vblank_min = vblank / 10;
				int vblank_max = vblank - vblank_min;
				
				// End Logic Timer
				sceKernelGetSystemTime(&clock_end);
				
				// Calculate Time wasted on Logic
				int calc_speed = (int)clock_end.low - (int)clock_start.low;
				
				// Start Rendering Timer
				sceKernelGetSystemTime(&clock_start);
				
				// Draw HUD
				if(hud_on) drawInfo(&displayCanvas);
				
				// Draw Chat Notification
				else drawNotification(&displayCanvas);
				
				// End Rendering Timer
				sceKernelGetSystemTime(&clock_end);
				
				// Calculate Time wasted on Rendering
				int draw_speed = (int)clock_end.low - (int)clock_start.low;
				
				// Calculate Vertical Blank Delay
				int delay = vblank - draw_speed*3 - calc_speed;
				if(delay < vblank_min) delay = vblank_min;
				if(delay > vblank_max) delay = vblank_max;
				
				// Apply Vertical Blank Delay
				sceKernelDelayThread(delay);
				
				// Draw HUD
				if(hud_on) drawInfo(&displayCanvas);
				
				// Draw Chat Notification
				else drawNotification(&displayCanvas);
			}
			
			// Unblock Drawing Operation
			wait = 0;
		}
		
		// Standard Thread Delay
		sceKernelDelayThread(10000);
	}
	
	// Clear Running Status
	running = 0;
	
	// Kill Thread
	sceKernelExitDeleteThread(0);
	
	// Return to Caller
	return 0;
}

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a)
#define GET_JUMP_TARGET(x) (0x80000000 | (((x) & 0x03FFFFFF) << 2))
#define HIJACK_FUNCTION(a, f, ptr) \
{ \
	printk("hijacking function at 0x%lx with 0x%lx\n", (u32)a, (u32)f); \
	u32 _func_ = (u32)a; \
	u32 _ff = (u32)f; \
	int _interrupts = pspSdkDisableInterrupts(); \
	sceKernelDcacheWritebackInvalidateAll(); \
	static u32 patch_buffer[3]; \
	_sw(_lw(_func_), (u32)patch_buffer); \
	_sw(_lw(_func_ + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP((u32)patch_buffer + 4, _func_ + 8); \
	_sw(0x08000000 | (((u32)(_ff) >> 2) & 0x03FFFFFF), _func_); \
	_sw(0, _func_ + 4); \
	ptr = (void *)patch_buffer; \
	sceKernelDcacheWritebackInvalidateAll(); \
	sceKernelIcacheClearAll(); \
	pspSdkEnableInterrupts(_interrupts); \
	printk("original instructions: 0x%lx 0x%lx\n", _lw((u32)patch_buffer), _lw((u32)patch_buffer + 8)); \
}

int volatile_locked = 0;

#define USE_REAL_VOLATILE_MEMLOCK 0
s32 sceKernelVolatileMemLock(s32 unk, void **ptr, s32 *size);
s32 (*sceKernelVolatileMemLockOrig)(s32 unk, void **ptr, s32 *size) = NULL;
s32 sceKernelVolatileMemLockPatched(s32 unk, void **ptr, s32 *size)
{
	#if USE_REAL_VOLATILE_MEMLOCK
	s32 result = sceKernelVolatileMemLockOrig(unk, ptr, size);
	printk("%s: 0x%x 0x%x/0x%x 0x%x/%d, 0x%x\n", __func__, unk, ptr, *ptr, size, *size, result);
	return result;
	#else
	//printk("%s: 0x%x, 0x%x, 0x%x/%d\n", __func__, unk, ptr, size, *size);
	*ptr = (void *)0x08400000;
	*size = 4194304;
	volatile_locked = 1;
	return 0;
	#endif
}
s32 sceKernelVolatileMemTryLock(s32 unk, void **ptr, s32 *size);
s32 (*sceKernelVolatileMemTryLockOrig)(s32 unk, void **ptr, s32 *size) = NULL;
s32 sceKernelVolatileMemTryLockPatched(s32 unk, void **ptr, s32 *size)
{
	#if USE_REAL_VOLATILE_MEMLOCK
	s32 result = sceKernelVolatileMemTryLockOrig(unk, ptr, size);
	printk("%s: 0x%x 0x%x/0x%x 0x%x/%d, 0x%x\n", __func__, unk, ptr, *ptr, size, *size, result);
	return result;
	#else
	//printk("%s: 0x%x, 0x%x, 0x%x/%d\n", __func__, unk, ptr, size, *size);
	*ptr = (void *)0x08400000;
	*size = 4194304;
	volatile_locked = 1;
	return 0;
	#endif
}

s32 sceKernelVolatileMemUnlock(s32 unk);
s32 (*sceKernelVolatileMemUnlockOrig)(s32 unk) = NULL;
s32 sceKernelVolatileMemUnlockPatched(s32 unk)
{
	#if USE_REAL_VOLATILE_MEMLOCK
	s32 result = sceKernelVolatileMemUnlockOrig(unk);
	printk("%s: 0x%x, 0x%x\n", __func__, unk, result);
	return result;
	#else
	//printk("%s: 0x%x\n", __func__, unk);
	volatile_locked = 0;
	return 0;
	#endif
}

#define USE_REAL_POWER_LOCK 0

s32 sceKernelPowerLock(s32 lockType);
s32 (*sceKernelPowerLockOrig)(s32 lockType);
s32 sceKernelPowerLockPatched(s32 lockType)
{
	#if USE_REAL_POWER_LOCK
	s32 result = sceKernelPowerLockOrig(lockType);
	printk("%s: power lock 0x%x, 0x%x\n", __func__, lockType, result);
	return result;
	#else
	printk("%s: power lock 0x%x\n", __func__, lockType);
	return 0;
	#endif
}

s32 sceKernelPowerUnlock(s32 lockType);
s32 (*sceKernelPowerUnlockOrig)(s32 lockType);
s32 sceKernelPowerUnlockPatched(s32 lockType)
{
	#if USE_REAL_POWER_LOCK
	s32 result = sceKernelPowerUnlockOrig(lockType);
	printk("%s: power unlock 0x%x, 0x%x\n", __func__, lockType, result);
	return result;
	#else
	printk("%s: power unlock 0x%x\n", __func__, lockType);
	return 0;
	#endif
}

s32 sceKernelPowerLockForUser(s32 lockType);
s32 (*sceKernelPowerLockForUserOrig)(s32 lockType);
s32 sceKernelPowerLockForUserPatched(s32 lockType)
{
	#if USE_REAL_POWER_LOCK
	s32 result = sceKernelPowerLockForUserOrig(lockType);
	printk("%s: power lock 0x%x, 0x%x\n", __func__, lockType, result);
	return result;
	#else
	//printk("%s: power lock 0x%x\n", __func__, lockType);
	return 0;
	#endif
}

s32 sceKernelPowerUnlockForUser(s32 lockType);
s32 (*sceKernelPowerUnlockForUserOrig)(s32 lockType);
s32 sceKernelPowerUnlockForUserPatched(s32 lockType)
{
	#if USE_REAL_POWER_LOCK
	s32 result = sceKernelPowerUnlockForUserOrig(lockType);
	printk("%s: power unlock 0x%x, 0x%x\n", __func__, lockType, result);
	return result;
	#else
	//printk("%s: power unlock 0x%x\n", __func__, lockType);
	return 0;
	#endif
}

// Module Start Event
int module_start(SceSize args, void * argp)
{
	// Result
	int result = 0;

	// Initialize Logfile
	printk_init("ms0:/atpro.log");

	// Alive Message
	printk("ATPRO - ALPHA VERSION %s %s\n", __DATE__, __TIME__);
	
	// Enable Online Mode
	onlinemode = sceWlanGetSwitchState();
	
	// Log WLAN Switch State
	printk("WLAN Switch: %d\n", onlinemode);
	
	// Grab API Type
	// int api = sceKernelInitApitype();
	
	// Game Mode & WLAN Switch On
	if(sceKernelInitKeyConfig() == PSP_INIT_KEYCONFIG_GAME) {
	// if(api == 0x120 || api == 0x123 || api == 0x125) {
		// Find Utility Manager
		SceModule * utility = sceKernelFindModuleByName("sceUtility_Driver");
		printk("sceUtility_Driver Scan: %08X\n", (u32)utility);
		if(utility != NULL) {
			// 6.20 NIDs
			#ifdef CONFIG_620
			u32 nid[] = { 0xE3CCC6EA, 0x07290699, 0x9CEB18C4, 0xDF8FFFAB };
			#endif

			// 6.35 NIDs
			#ifdef CONFIG_63X
			u32 nid[] = { 0xFFB9B760, 0xAFBC3911, 0x0D053026, 0xE6BF3960 };
			#endif

			// 6.60 NIDs
			#ifdef CONFIG_660
			u32 nid[] = { 0x939E4270, 0xD4EE2D26, 0x387E3CA9, 0x3FF74DF1, 0xE5D6087B};
			#endif

			// Patch Utility Manager Imports
			hook_import_bynid(utility, "ModuleMgrForKernel", nid[4], stop_plugin_kernel);
			hook_import_bynid(utility, "ModuleMgrForKernel", nid[3], start_plugin_kernel);
			hook_import_bynid(utility, "ModuleMgrForKernel", nid[2], unload_plugin_kernel);
			result = hook_import_bynid(utility, "ModuleMgrForKernel", nid[0], load_plugin_kernel);
			printk("Kernel Loader Hook: %d\n", result);
			if(result == 0) {
				result = hook_import_bynid(utility, "ModuleMgrForKernel", nid[1], load_plugin_alt);
				printk("User Loader Hook: %d\n", result);
				if(result == 0) {
					// Disable Home Menu
					sctrlHENPatchSyscall((void*)sctrlHENFindFunction("sceLoadExec", "LoadExecForUser", 0x4AC57943), cbdeny);
					printk("Disabled Home Menu!\n");
					
					// Enable Module Start Patching
					sysctrl_patcher = sctrlHENSetStartModuleHandler(online_patcher);
					printk("Enabled Game-Specific Fixes!\n");
					
					// Disable Sleep Mode to prevent Infrastructure Death
					if(onlinemode)
					{
						// Disable Sleep Mode
						scePowerLock(0);
						printk("Disabled Power Button!\n");

						// Monitor/disable volatile lock
						HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelVolatileMemLock), sceKernelVolatileMemLockPatched, sceKernelVolatileMemLockOrig);
						HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelVolatileMemTryLock), sceKernelVolatileMemTryLockPatched, sceKernelVolatileMemTryLockOrig);
						HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelVolatileMemUnlock), sceKernelVolatileMemUnlockPatched, sceKernelVolatileMemUnlockOrig);

						// Monitor power locks
						//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerLock), sceKernelPowerLockPatched, sceKernelPowerLockOrig);
						//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerUnlock), sceKernelPowerUnlockPatched, sceKernelPowerUnlockOrig);
						HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerLockForUser), sceKernelPowerLockForUserPatched, sceKernelPowerLockForUserOrig);
						HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerUnlockForUser), sceKernelPowerUnlockForUserPatched, sceKernelPowerUnlockForUserOrig);

						// Monitor netconf init
						void *netconf_init_func = (void *)sctrlHENFindFunction("sceUtility_Driver", "sceUtility", 0x4DB1E739);
						HIJACK_FUNCTION(netconf_init_func, netconf_init, netconf_init_orig);

					}
					
					// Create Input Thread
					int ctrl = sceKernelCreateThread("atpro_input", input_thread, 0x10, 32768, 0, NULL);
					
					// Created Input Thread
					if(ctrl >= 0)
					{
						// Set Running Flag for Input Thread
						running = 1;
						
						// Start Input Thread
						sceKernelStartThread(ctrl, 0, NULL);
					}

					// Setup Success
					return 0;
				}
			}
		}
	}

	// Setup Failure
	return 1;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	// Shutdown GUI
	running = -1;
	
	// Wait for GUI Shutdown
	while(running != 0) sceKernelDelayThread(10000);
	
	// Return Success
	return 0;
}
