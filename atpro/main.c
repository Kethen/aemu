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
#include <stdbool.h>
#include "libs.h"
#include "hud.h"
#include "logs.h"
#include "systemctrl.h"

PSP_MODULE_INFO("ATPRO", PSP_MODULE_KERNEL, 1, 1);

// Game Code Getter (discovered in utility.prx)
const char * SysMemGameCodeGetter(void);

// System Control Module Patcher
STMOD_HANDLER sysctrl_patcher = NULL;

// Display Canvas
CANVAS displayCanvas = {0};

// Input Thread Running Flag
int running = 0;

// HUD Overlay Flag
int hud_on = 0;

// Render-Wait State Flag
int wait = 0;

// Frame Counter
//int framecount = 0;

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

char * module_build_names[MODULE_LIST_SIZE] = {
	"sceMemab",
	"sceNetAdhocAuth_Service",
	"sceNetAdhoc_Library",
	"sceNetAdhocctl_Library",
	"sceNetAdhocMatching_Library"
};

const char *force_fw_modules[] = {
	"ifhandle.prx",
	"pspnet.prx",
	"pspnet_inet.prx",
	"pspnet_apctl.prx",
	"pspnet_resolver.prx",
};

const char *force_fw_module_names[sizeof(force_fw_modules) / sizeof(force_fw_modules[0])] = {
	"sceNet_Service",
	"sceNet_Library",
	"sceNetInet_Library",
	"sceNetApctl_Library",
	"sceNetResolver_Library",
};

const char *late_load_modules[] = {
	"ifhandle.prx",
	"pspnet.prx",
	"pspnet_inet.prx",
	"pspnet_apctl.prx",
	"pspnet_resolver.prx"
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
	SceUID uid = sceKernelAllocPartitionMemory(2, "aemu allocation", 4 /* high aligned */, size, (void *)4);

	if (uid < 0)
	{
		printk("%s: failed allocating %d low aligned, 0x%x\n", __func__, size, uid);
		return NULL;
	}

	return sceKernelGetBlockHeadAddr(uid);
}

static int is_standalone(){
	int test_file = sceIoOpen("flash0:/kd/kermit.prx", PSP_O_RDONLY, 0777);
	if (test_file >= 0){
		sceIoClose(test_file);
		return 1;
	}
	return 0;
}

static int is_vita(){
	int test_file = sceIoOpen("flash0:/kd/usb.prx", PSP_O_RDONLY, 0777);
	if (test_file >= 0){
		sceIoClose(test_file);
		return 0;
	}
	return 1;
}

static int is_go(){
	return sceKernelGetModel() == 4;
}

int has_high_mem(){
	return is_vita() || sceKernelGetModel() != 0;
}

void steal_memory()
{
	int size = 8 * 1024 * 1024;
	size += 3 * 1024 * 1024; // PSP go odd memory layout

	if (!has_high_mem()){
		size = 1024 * 1024 * 2; // we're boned if the game wants more than this initially
	}

	if (stolen_memory >= 0)
	{
		printk("%s: refuse to steal memory again\n", __func__);
		return;
	}

	stolen_memory = sceKernelAllocPartitionMemory(2, "inet apctl load reserve", PSP_SMEM_High, size, NULL);
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

static char tolower(char value)
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
	"sceNetResolver_Library",
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
	"pspnet_resolver.prx",
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
	return load_plugin(path, flags, option, sceKernelLoadModule);
}
SceUID load_plugin_user(const char * path, int flags, SceKernelLMOption * option)
{
	if (load_plugin_user_orig == NULL)
	{
		load_plugin_user_orig = (module_loader_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0x977DE386);
	}

	if (option == NULL)
	{
		printk("%s: loading %s without options\n", __func__, path);
	}
	else
	{
		printk("%s: loading %s into partition %d/%d with position %d\n", __func__, path, option->mpidtext, option->mpiddata, option->position);
	}

	return load_plugin(path, flags, option, load_plugin_user_orig);
}
SceUID load_plugin(const char * path, int flags, SceKernelLMOption * option, module_loader_func orig)
{
	// Force module path case
	char test_path[256] = {0};

	int len = strlen(path);
	for(int i = 0;i < len;i++){
		test_path[i] = tolower(path[i]);
	}

	while (strstr(path, "disc0:/sce_lbn") != NULL){
		SceUID modid = orig(path, 0, NULL);
		if (modid < 0){
			uint32_t k1 = pspSdkSetK1(0);
			modid = sceKernelLoadModule(path, 0, NULL);
			pspSdkSetK1(k1);
		}
		if (modid < 0){
			//printk("%s: failed loading %s as a module, 0x%x\n", __func__, path, modid);
			break;
		}

		SceKernelModuleInfo info = {0};
		info.size = sizeof(info);

		uint32_t k1 = pspSdkSetK1(0);
		int query_status = sceKernelQueryModuleInfo(modid, &info);
		pspSdkSetK1(k1);
		if (query_status != 0){
			//printk("%s: failed fetching module info of %s\n", __func__, path);
			sceKernelUnloadModule(modid);
			break;
		}

		printk("%s: module name of %s is %s\n", __func__, path, info.name);

		sceKernelUnloadModule(modid);

		for (int i = 0;i < sizeof(module_build_names) / sizeof(module_build_names[0]);i++){
			if (strcmp(info.name, module_build_names[i]) == 0){
				sprintf(test_path, "disc0:/kd/%s", module_names[i]);
				printk("%s: %s -> %s\n", __func__, path, test_path);
				break;
			}
		}

		for (int i = 0;i < sizeof(force_fw_module_names) / sizeof(force_fw_module_names[0]);i++){
			if (strcmp(info.name, force_fw_module_names[i]) == 0){
				sprintf(test_path, "disc0:/kd/%s", force_fw_modules[i]);
				printk("%s: %s -> %s\n", __func__, path, test_path);
				break;
			}
		}

		break;
	}

	printk("%s: test path %s\n", __func__, test_path);

	// Online Mode Enabled
	if(onlinemode)
	{
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
			if (strstr(test_path, no_unload_module_file_names[i]) != NULL)
			{
				if (no_unload_module_uids[i] >= 0)
				{
					printk("%s: returning previous uid 0x%x for %s\n", __func__, no_unload_module_uids[i], path);
					return no_unload_module_uids[i];
				}
			}
		}

		//if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
		{
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
				
				//if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
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
						printk("%s: loaded no unload module %s\n", __func__, no_unload_module_file_names[i]);
						no_unload_module_uids[i] = result;
						break;
					}
				}

				// Return Module UID
				return result;
			}
		}

		// Load shim if needed
		static const char *shimmed_modules[] = {
			"pspnet_apctl.prx",
			"pspnet_resolver.prx"
		};

		for(int i = 0;i < sizeof(shimmed_modules) / sizeof(char *);i++)
		{
			if (strstr(test_path, shimmed_modules[i]) != NULL && shim_uid < 0)
			{
				if (shim_uid < 0)
				{
					shim_uid = load_start_module(shim_path);
					break;
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
		if (strstr(test_path, no_unload_module_file_names[i]))
		{
			printk("%s: loaded no unload module %s\n", __func__, no_unload_module_file_names[i]);
			no_unload_module_uids[i] = result;
			break;
		}
	}

	return result;
}

struct fd_path_map_entry{
	SceUID fd;
	char path[256];
};

struct fd_path_map_entry fd_path_map[16] = {0};

static void add_fd_path_map_entry(const char *path, SceUID fd){
	int slot = -1;
	int interrupts = pspSdkDisableInterrupts();
	for (int i = 0;i < sizeof(fd_path_map) / sizeof(fd_path_map[0]);i++){
		if (fd_path_map[i].fd == -1){
			slot = i;
			break;
		}
	}
	if (slot == -1){
		pspSdkEnableInterrupts(interrupts);
		printk("%s: could not find a free slot for tracking 0x%x %s\n", __func__, fd, path);
		return;
	}
	fd_path_map[slot].fd = fd;
	strcpy(fd_path_map[slot].path, path);
	pspSdkEnableInterrupts(interrupts);
	return;
}

static void remove_fd_path_map_entry(SceUID fd){
	int interrupts = pspSdkDisableInterrupts();
	for (int i = 0;i < sizeof(fd_path_map) / sizeof(fd_path_map[0]);i++){
		if (fd_path_map[i].fd == fd){
			fd_path_map[i].fd = -1;
			pspSdkEnableInterrupts(interrupts);
			return;
		}
	}
	pspSdkEnableInterrupts(interrupts);
}

static int get_fd_path(SceUID fd, char *path){
	int interrupts = pspSdkDisableInterrupts();
	for (int i = 0;i < sizeof(fd_path_map) / sizeof(fd_path_map[0]);i++){
		if (fd_path_map[i].fd == fd){
			strcpy(path, fd_path_map[i].path);
			pspSdkEnableInterrupts(interrupts);
			return 1;
		}
	}
	pspSdkEnableInterrupts(interrupts);
	return 0;
}

typedef SceUID (*load_module_by_id_func)(SceUID fd, int flags, SceKernelLMOption *option);
load_module_by_id_func load_module_by_id_orig = NULL;

struct known_func{
	const char *module_name;
	const char *library_name;
	uint32_t nid;
};

static struct known_func known_open_funcs[] = {
	//{.module_name = "mhp3patch", .library_name = "mhp3kernel", .nid = 0x45ACEAF2}, // codestation's monster hunter patch loader
	{.module_name = "mhp3patch", .library_name = "mhp3kernel", .nid = 0x0},
	//{.module_name = "divapatch", .library_name = "divakernel", .nid = 0xDA93ACA2}, // codestation's diva patch loader
	{.module_name = "divapatch", .library_name = "divakernel", .nid = 0x0},
	{.module_name = "nploader", .library_name = "nploader", .nid = 0x333A34AE}, // nploader
	{.module_name = "stargate", .library_name = "stargate", .nid = 0x7C8EFE7D}, // procfw stargate
	{.module_name = "sceIOFileManager", .library_name = "IoFileMgrForUser", .nid = 0x109F50BC}, // normal sceIoOpen
};

struct known_func known_close_funcs[] = {
	//{.module_name = "mhp3patch", .library_name = "mhp3kernel", .nid = 0x35FFD283}, // codestation's monster hunter patch loader
	{.module_name = "mhp3patch", .library_name = "mhp3kernel", .nid = 0x0},
	//{.module_name = "divapatch", .library_name = "divakernel", .nid = 0xCAC4B65D}, // codestation's diva patch loader
	{.module_name = "divapatch", .library_name = "divakernel", .nid = 0x0},
	{.module_name = "sceIOFileManager", .library_name = "IoFileMgrForUser", .nid = 0x810C4BC3}, // normal sceIoClose
};

typedef SceUID (*open_func)(const char *path, int flags, SceMode mode);
open_func open_orig = NULL;
SceUID open_file(const char *path, int flags, SceMode mode){
	for(int i = 0;open_orig == NULL && i < sizeof(known_open_funcs) / sizeof(known_open_funcs[0]);i++){
		open_orig = (void *)sctrlHENFindFunction(known_open_funcs[i].module_name, known_open_funcs[i].library_name, known_open_funcs[i].nid);
		if (open_orig != NULL){
			printk("%s: using %s %s 0x%x for sceIoOpen\n", __func__, known_open_funcs[i].module_name, known_open_funcs[i].library_name, known_open_funcs[i].nid);
		}
	}

	SceUID fd = open_orig(path, flags, mode);
	if (fd < 0){
		return fd;
	}

	while (strstr(path, "disc0:/sce_lbn") != NULL){
		// peek the magic... actually no it can't read that way it seems, for sce_lbn paths
		#if 0
		uint8_t magic[4];
		sceIoLseek(fd, 0, PSP_SEEK_SET);
		sceIoRead(fd, magic, sizeof(magic));
		sceIoLseek(fd, 0, PSP_SEEK_SET);
		static const uint8_t raw_prx_magic[] = {0x7f, 0x45, 0x4c, 0x46};
		static const uint8_t normal_prx_magic[] = {0x7e, 0x50, 0x53, 0x50};
		if (memcmp(magic, raw_prx_magic, sizeof(magic)) != 0 && memcmp(magic, normal_prx_magic, sizeof(magic)) != 0){
			printk("%s: file is not a prx, 0x%x 0x%x 0x%x 0x%x\n", __func__, (uint32_t)magic[0], (uint32_t)magic[1], (uint32_t)magic[2], (uint32_t)magic[3]);
			break;
		}
		#endif

		if (load_module_by_id_orig == NULL){
			load_module_by_id_orig = (load_module_by_id_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0xB7F46618);
		}

		SceUID modid = load_module_by_id_orig(fd, 0, NULL);
		if (modid < 0){
			uint32_t k1 = pspSdkSetK1(0);
			modid = sceKernelLoadModuleByID(fd, 0, NULL);
			pspSdkSetK1(k1);
		}
		if (modid < 0){
			//printk("%s: failed loading 0x%x %s as a module, 0x%x\n", __func__, fd, path, modid);
			sceIoLseek(fd, 0, PSP_SEEK_SET);
			break;
		}

		SceKernelModuleInfo info = {0};
		info.size = sizeof(info);

		uint32_t k1 = pspSdkSetK1(0);
		int query_status = sceKernelQueryModuleInfo(modid, &info);
		pspSdkSetK1(k1);
		if (query_status != 0){
			//printk("%s: failed fetching module info of 0x%x %s\n", __func__, fd, path);
			sceIoLseek(fd, 0, PSP_SEEK_SET);
			sceKernelUnloadModule(modid);
			break;
		}

		printk("%s: module name of 0x%x %s is %s\n", __func__, fd, path, info.name);

		sceIoLseek(fd, 0, PSP_SEEK_SET);
		sceKernelUnloadModule(modid);

		for (int i = 0;i < sizeof(module_build_names) / sizeof(module_build_names[0]);i++){
			if (strcmp(info.name, module_build_names[i]) == 0){
				char full_path[128] = {0};
				sprintf(full_path, "disc0:/kd/%s", module_names[i]);
				printk("%s: adding 0x%x %s to fd path map as %s\n", __func__, fd, path, full_path);
				add_fd_path_map_entry(full_path, fd);
				return fd;
			}
		}

		for (int i = 0;i < sizeof(force_fw_module_names) / sizeof(force_fw_module_names[0]);i++){
			if (strcmp(info.name, force_fw_module_names[i]) == 0){
				char full_path[128] = {0};
				sprintf(full_path, "disc0:/kd/%s", force_fw_modules[i]);
				printk("%s: adding 0x%x %s to fd path map as %s\n", __func__, fd, path, full_path);
				add_fd_path_map_entry(full_path, fd);
				return fd;
			}
		}

		break;
	}

	int len = strlen(path);
	if (len < 4){
		//printk("%s: not tracking 0x%x %s\n", __func__, fd, path);
		return fd;
	}

	char extension[4];
	for (int i = 0;i < 4;i++){
		extension[i] = tolower(path[len - (4 - i)]);
	}

	if (memcmp(extension, ".prx", 4) != 0){
		//printk("%s: not tracking 0x%x %s\n", __func__, fd, path);
		return fd;
	}

	printk("%s: adding 0x%x 0x%x 0x%x %s to fd path map\n", __func__, fd, flags, mode, path);
	add_fd_path_map_entry(path, fd);
	return fd;
}

typedef int (*close_func)(SceUID fd);
close_func close_orig = NULL;
int close_file(SceUID fd){
	for(int i = 0;close_orig == NULL && i < sizeof(known_close_funcs) / sizeof(known_close_funcs[0]);i++){
		close_orig = (void *)sctrlHENFindFunction(known_close_funcs[i].module_name, known_close_funcs[i].library_name, known_close_funcs[i].nid);
		if (close_orig != NULL){
			printk("%s: using %s %s 0x%x for sceIoClose\n", __func__, known_close_funcs[i].module_name, known_close_funcs[i].library_name, known_close_funcs[i].nid);
		}
	}

	remove_fd_path_map_entry(fd);
	return close_orig(fd);
}

SceUID load_module_by_id(SceUID fd, int flags, SceKernelLMOption *option){
	if (load_module_by_id_orig == NULL){
		load_module_by_id_orig = (load_module_by_id_func)sctrlHENFindFunction("sceModuleManager", "ModuleMgrForUser", 0xB7F46618);
	}
	char path[256] = {0};
	if (!get_fd_path(fd, path)){
		printk("%s: untracked module fd 0x%x!\n", __func__, fd);
		SceUID result = load_module_by_id_orig(fd, flags, option);
		printk("%s: load result 0x%x\n", __func__, result);
		return result;
	}

	// we got a path

	#if 1
	// check if we should mess with the module at all
	// Force module path case
	bool should_redirect = false;

	int len = strlen(path);
	for(int i = 0;i < len;i++){
		path[i] = tolower(path[i]);
	}

	for (int i = 0;i < sizeof(module_names) / sizeof(module_names[0]);i++){
		if(strstr(path, module_names[i]) != NULL){
			should_redirect = true;
			break;
		}
	}

	for(int i = 0;i < sizeof(force_fw_modules) / sizeof(force_fw_modules[0]) && !should_redirect;i++){
		if(strstr(path, force_fw_modules[i]) != NULL){
			should_redirect = true;
			break;
		}
	}

	// restore the path
	get_fd_path(fd, path);

	if (!should_redirect){
		printk("%s: not interfering with the loading of %s 0x%x\n", __func__, path, fd);
		SceUID result = load_module_by_id_orig(fd, flags, option);
		printk("%s: load result 0x%x\n", __func__, result);
		return result;
	}
	#endif

	//SceUID result = load_plugin_user(path, flags, option);
	SceUID result = load_plugin_user(path, 0, NULL);
	printk("%s: load result 0x%x\n", __func__, result);
	return result;
}

typedef int (*module_unload_func)(SceUID uid);
int unload_plugin(SceUID uid, module_unload_func);
int unload_plugin_kernel(SceUID uid){
	//printk("%s: begin\n", __func__);
	return unload_plugin(uid, sceKernelUnloadModule);
}
module_unload_func unload_plugin_user_orig = NULL;
int unload_plugin_user(SceUID uid)
{
	//printk("%s: begin\n", __func__);
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
		//printk("%s: failed fetching module name\n", __func__);
		return orig(uid);
	}

	//printk("%s: unloading %s\n", __func__, info.name);

	for(int i = 0;i < sizeof(no_unload_modules) / sizeof(char *) && onlinemode;i++){
		// Stop these modules from being unloaded
		if (strstr(info.name, no_unload_modules[i]) != NULL){
			//printk("%s: blocked %s unloading\n", __func__, no_unload_modules[i]);
			return 0;
		}
	}

	return orig(uid);
}

typedef int (*module_start_func)(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option);
int start_plugin(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option, module_start_func);
int start_plugin_kernel(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option){
	printk("%s: begin\n", __func__);
	return start_plugin(uid, argsize, argp, status, option, sceKernelStartModule);
}
module_start_func start_plugin_user_orig = NULL;
int start_plugin_user(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	printk("%s: begin\n", __func__);
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
		int result = orig(uid, argsize, argp, status, option);
		printk("%s: failed fetching module name, started with result 0x%x/%d\n", __func__, result, result);
		return result;
	}

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

	int result = orig(uid, argsize, argp, status, option);
	printk("%s: started %s, 0x%x/%d\n", __func__, info.name, result, result);
	return result;
}

typedef int (*module_stop_func)(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option);
int stop_plugin(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option, module_stop_func);
int stop_plugin_kernel(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option){
	//printk("%s: begin\n", __func__);
	return stop_plugin(uid, argsize, argp, status, option, sceKernelStopModule);
}
module_stop_func stop_plugin_user_orig = NULL;
int stop_plugin_user(SceUID uid, SceSize argsize, void *argp, int *status, SceKernelSMOption *option)
{
	//printk("%s: begin\n", __func__);
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
		int result = orig(uid, argsize, argp, status, option);
		//printk("%s: failed fetching module name, stopped with 0x%x/%d\n", __func__, result, result);
		return result;
	}

	for(int i = 0;i < sizeof(no_unload_modules) / sizeof(char *) && onlinemode;i++){
		// Do not stop these modules
		if (strstr(info.name, no_unload_modules[i]) != NULL){
			//printk("%s: blocked stopping of %s\n", __func__, no_unload_modules[i]);
			return 0;
		}
	}

	int result = orig(uid, argsize, argp, status, option);
	//printk("%s: stopped %s, 0x%x/%d\n", __func__, info.name, result, result);
	return result;
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

				//if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
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

				//if (sceKernelGetSystemTimeWide() - game_begin > LOAD_RETURN_MEMORY_THRES_USEC)
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

void get_game_code(char *buf, int len){
	memset(buf, 0, len);
	strncpy(buf, getGameCode(), len - 1);
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

static char nickname[128] = {0};
int get_system_param_string(int id, char *str, int len)
{
	if (id == PSP_SYSTEMPARAM_ID_STRING_NICKNAME){

		if (nickname[0] == '\0')
		{
			uint32_t k1 = pspSdkSetK1(0);
			int fetch_status = sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, nickname, sizeof(nickname));
			nickname[127] = '\0';
			pspSdkSetK1(k1);
		}

		if (nickname[0] == '\0')
		{
			sprintf(nickname, "AEMU %u", sceKernelGetSystemTimeLow() % 125);
		}

		if (len > 0){
			strncpy(str, nickname, len);
			str[len - 1] = '\0';
		}
		return 0;
	}
	return sceUtilityGetSystemParamString(id, str, len);
}

pspUtilityNetconfData *netconf_override;
struct pspUtilityNetconfAdhoc *netconf_adhoc_override;
pspUtilityNetconfData *orig_data = NULL;
int (*netconf_init_orig)(pspUtilityNetconfData *data) = NULL;
int netconf_init(pspUtilityNetconfData *data){
	if (data != NULL)
	{
		printk("%s: data size is %d, expected %d\n", __func__, data->base.size, sizeof(pspUtilityNetconfData));
	}

	orig_data = NULL;
	if (data != NULL && netconf_override != NULL && netconf_adhoc_override != NULL && data->action == PSP_NETCONF_ACTION_CONNECTAP)
	{
		printk("%s: overriding netconf param for infra mode\n", __func__);
		orig_data = data;
		data = netconf_override;

		int ctrl = 1;
		int lang = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
		sceUtilityGetSystemParamInt(9, &ctrl);
		sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &lang);

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

int (*netconf_get_status_orig)() = NULL;
int netconf_get_status()
{
	int result = netconf_get_status_orig();
	if (orig_data != NULL)
	{
		//printk("%s: copying netconf param from override to original\n", __func__);
		int size = orig_data->base.size;
		memcpy(orig_data, netconf_override, size);
		orig_data->base.size = size;
		//printk("%s: ret 0x%x result 0x%x\n", __func__, result, orig_data->base.result);
	}
	//printk("%s: returning %d/0x%x\n", __func__, result, result);
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

static void draw_hud()
{
	// Increase Frame Counter
	//framecount++;

	// Ready to Paint State
	if(wait == 0 && displayCanvas.buffer != NULL)
	{
		// Lock State
		wait = 1;
		
		// Get Canvas Information
		int mode = 0; sceDisplayGetMode(&mode, &(displayCanvas.width), &(displayCanvas.height));
		
		// HUD Painting Required
		if(hud_on) drawInfo(&displayCanvas);
		
		// Notification Painting Required
		else drawNotification(&displayCanvas);
		
		// Unlock State
		wait = 0;
	}
}

int sceDisplayWaitVblankPatched()
{
	draw_hud();
	return sceDisplayWaitVblank();
}

int sceDisplayWaitVblankCBPatched()
{
	draw_hud();
	return sceDisplayWaitVblankCB();
}

int sceDisplayWaitVblankStartPatched()
{
	draw_hud();
	return sceDisplayWaitVblankStart();
}

int sceDisplayWaitVblankStartCBPatched()
{
	draw_hud();
	return sceDisplayWaitVblankStartCB();
}

// Framebuffer Setter
int setframebuf(void *topaddr, int bufferwidth, int pixelformat, int sync)
{
	// Update Canvas Information
	displayCanvas.buffer = topaddr;
	displayCanvas.lineWidth = bufferwidth;
	displayCanvas.pixelFormat = pixelformat;

	draw_hud();

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

static void log_memory_info(){
	#ifdef DEBUG
	PspSysmemPartitionInfo meminfo = {0};
	meminfo.size = sizeof(PspSysmemPartitionInfo);
	for(int i = 1;i < 13;i++){
		int query_status = sceKernelQueryMemoryPartitionInfo(i, &meminfo);
		if (query_status == 0){
			int max_free = sceKernelPartitionMaxFreeMemSize(i);
			int total_free = sceKernelPartitionTotalFreeMemSize(i);
			printk("%s: p%d startaddr 0x%x size %d attr 0x%x max %d total %d\n", __func__, i, meminfo.startaddr, meminfo.memsize, meminfo.attr, max_free, total_free);
		}else{
			printk("%s: p%d query failed, 0x%x\n", __func__, i, query_status);
		}
	}
	#endif
}

static void early_memory_stealing()
{
	#if 0
	int memdump_1 = sceIoOpen("ms0:/memdump_8a000000.bin", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (memdump_1 >= 0){
		sceIoWrite(memdump_1, (void *)0x8a000000, 1024 * 1024 * 4);
		sceIoClose(memdump_1);
	}
	int memdump_2 = sceIoOpen("ms0:/memdump_8b000000.bin", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (memdump_2 >= 0){
		sceIoWrite(memdump_2, (void *)0x8b000000, 1024 * 1024 * 4);
		sceIoClose(memdump_2);
	}

	return;
	#endif


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

typedef struct PartitionData {
	u32 unk[5];
	u32 size;
} PartitionData;

typedef struct SysMemPartition {
	struct SysMemPartition *next;
	u32	address;
	u32 size;
	u32 attributes;
	PartitionData *data;
} SysMemPartition;

// based on Adrenaline
static void memlayout_hack(){
	if(!has_high_mem()){
		printk("%s: not slim/vita\n", __func__);
		return;
	}

	SysMemPartition *(*get_partition)() = NULL;
	for (u32 addr = 0x88000000;addr < 0x4000 + 0x88000000;addr+=4){
		if (_lw(addr) == 0x2C85000D){
		    get_partition = (SysMemPartition *(*)())(addr-4);
		    break;
		}
	}

	if (get_partition == NULL){
		printk("%s: can't find get_partition\n", __func__);
		return;
	}

	SysMemPartition *partition_2 = get_partition(2);
	SysMemPartition *partition_9 = get_partition(is_vita() ? 11 : 9);

	if (partition_9 == NULL){
		printk("%s: partition 9 not found\n", __func__);
		return;
	}

	// memory layout with just r6 loaded: log_memory_info: p2 startaddr 0x8800000 size 25165824 attr 0xf max 17314048 total 17314048

	#if 1
	// the goal is to have extra memory, but only offer the game roughly vanilla amount of memory after memory is stolen
	// also be very careful not to hit the 40 MB limit barrier on PSVita, there be dragons beyond 40 MB
	partition_2->size = 24 * 1024 * 1024; // base
	partition_2->size += 8 + 128; // 128 B of netconf param alloc
	partition_2->size += 8 * 1024 * 1024; // stolen
	partition_2->size += 3 * 1024 * 1024; // PSP go odd memory layout
	partition_2->size += 50 * 1024; // buffer
	#else
	// 40 MB is currently the limit on the vita, until all unsafe zones are mapped
	partition_2->size = 40 * 1024 * 1024;
	#endif
	partition_2->data->size = (((partition_2->size >> 8) << 9) | 0xFC);
	partition_9->size = 0 * 1024 * 1024;
	partition_9->address = 0x88800000 + partition_2->size;
	partition_9->data->size = (((partition_9->size >> 8) << 9) | 0xFC);

	// Change memory protection
	u32 *prot = (u32 *)0xBC000040;

	int i;
	for (i = 0; i < 0x10; i++) {
		prot[i] = 0xFFFFFFFF;
	}

	sceKernelDcacheWritebackInvalidateAll();
	sceKernelIcacheClearAll();

	printk("%s: changed partition layout\n", __func__);
}

// Online Module Start Patcher
int online_patcher(SceModule2 * module)
{
	// Try to do this before stargate
	int sysctrl_patcher_result = sysctrl_patcher(module);

	printk("%s: module start %s text_addr 0x%x\n", __func__, module->modname, module->text_addr);

	if (module->text_addr > 0x08800000 && module->text_addr < 0x08900000 && strcmp("opnssmp", module->modname) != 0)
	{
		// Very likely the game itself

		printk("%s: guessing this is the game, %s text_addr 0x%x\n", __func__, module->modname, module->text_addr);
		if (onlinemode)
		{
			printk("%s: hooking module load/unload by the game and reserving memory for inet\n", __func__);
			early_memory_stealing();
			hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x977DE386, load_plugin_user);
			hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x2E0911AA, unload_plugin_user);
			hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x50F0C1EC, start_plugin_user);
			hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0xD1FF982A, stop_plugin_user);
			hook_import_bynid((SceModule *)module, "IoFileMgrForUser", 0x109F50BC, open_file);
			hook_import_bynid((SceModule *)module, "IoFileMgrForUser", 0x810C4BC3, close_file);
			hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0xB7F46618, load_module_by_id);

			// allocate memory for netconf
			//netconf_override = allocate_partition_memory(sizeof(allocate_partition_memory));
			//netconf_adhoc_override = allocate_partition_memory(sizeof(struct pspUtilityNetconfAdhoc));
			netconf_override = allocate_partition_memory(128);
			netconf_adhoc_override = (void *)(((uint32_t)netconf_override) + 72);

			if (strcmp(module->modname, "MonsterHunterPortable3rd") == 0){
				//{.module_name = "mhp3patch", .library_name = "mhp3kernel", .nid = 0x45ACEAF2}, // codestation's monster hunter patch loader
				for (int i = 0;i < sizeof(known_open_funcs) / sizeof(known_open_funcs[0]);i++){
					if (strcmp(known_open_funcs[i].module_name, "mhp3patch") == 0){
						known_open_funcs[i].nid = 0x45ACEAF2;
						break;
					}
				}

				//{.module_name = "mhp3patch", .library_name = "mhp3kernel", .nid = 0x35FFD283}, // codestation's monster hunter patch loader
				for (int i = 0;i < sizeof(known_close_funcs) / sizeof(known_close_funcs[0]);i++){
					if (strcmp(known_close_funcs[i].module_name, "mhp3patch") == 0){
						known_close_funcs[i].nid = 0x35FFD283;
						break;
					}
				}
			}

			if (strcmp(module->modname, "PdvApp") == 0){
				//{.module_name = "divapatch", .library_name = "divakernel", .nid = 0xDA93ACA2}, // codestation's diva patch loader
				for (int i = 0;i < sizeof(known_open_funcs) / sizeof(known_open_funcs[0]);i++){
					if (strcmp(known_open_funcs[i].module_name, "divapatch") == 0){
						known_open_funcs[i].nid = 0xDA93ACA2;
						break;
					}
				}

				//{.module_name = "divapatch", .library_name = "divakernel", .nid = 0xCAC4B65D}, // codestation's diva patch loader
				for (int i = 0;i < sizeof(known_close_funcs) / sizeof(known_close_funcs[0]);i++){
					if (strcmp(known_close_funcs[i].module_name, "divapatch") == 0){
						known_close_funcs[i].nid = 0xCAC4B65D;
						break;
					}
				}
			}
		}

		log_memory_info();

		printk("%s: hooking hud drawing and input\n", __func__);

		hook_import_bynid((SceModule *)module, "sceDisplay", 0x289D82FE, setframebuf);
		hook_import_bynid((SceModule *)module, "sceDisplay", 0x36CDFADE, sceDisplayWaitVblankPatched);
		hook_import_bynid((SceModule *)module, "sceDisplay", 0x8EB9EC49, sceDisplayWaitVblankCBPatched);
		hook_import_bynid((SceModule *)module, "sceDisplay", 0x984C27E7, sceDisplayWaitVblankStartPatched);
		hook_import_bynid((SceModule *)module, "sceDisplay", 0x46F186C3, sceDisplayWaitVblankStartCBPatched);

		hook_import_bynid((SceModule *)module, "sceCtrl", 0x1F803938, read_buffer_positive);
		hook_import_bynid((SceModule *)module, "sceCtrl", 0x3A622550, peek_buffer_positive);
		hook_import_bynid((SceModule *)module, "sceCtrl", 0x60B81F86, read_buffer_negative);
		hook_import_bynid((SceModule *)module, "sceCtrl", 0xC152080A, peek_buffer_negative);
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
			if (strstr(module->modname, "sceNetApctl_Library"))
			{
				void (*hijack_sceNetApctlInit)() = (void (*)())sctrlHENFindFunction("pspnet_shims", "pspnet_shims", 0x1);
				if (hijack_sceNetApctlInit != NULL)
				{
					printk("%s: redirecting sceNetApctlInit\n", __func__);
					hijack_sceNetApctlInit();					
				}else{
					printk("%s: hijack_sceNetApctlInit is null!\n", __func__);
				}
			}
			else if (strstr(module->modname, "sceNetResolver_Library"))
			{
				void (*hijack_sceNetResolver)() = (void (*)())sctrlHENFindFunction("pspnet_shims", "pspnet_shims", 0x2);
				if (hijack_sceNetResolver != NULL)
				{
					printk("%s: redirecting sceNetResolverInit and sceNetResolverTerm\n", __func__);
					hijack_sceNetResolver();
				}else{
					printk("%s: hijack_sceNetResolver is null!\n", __func__);
				}
			}

			// Lie to the game about adhoc channel for at least Ridge Racer 2
			hook_import_bynid((SceModule *)module, "sceUtility", 0xA5DA2406, get_system_param_int);

			// Inject placeholder nickname is empty
			hook_import_bynid((SceModule *)module, "sceUtility", 0x34B78343, get_system_param_string);
		}
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

		// New hud hotkey for standalone ARK
		if(!is_exit_button_pressed &&
			(prev_buttons & PSP_CTRL_SELECT) == 0 && (curr_buttons & PSP_CTRL_SELECT) != 0 &&
			(curr_buttons & PSP_CTRL_LTRIGGER) != 0 &&
			(curr_buttons & PSP_CTRL_RTRIGGER) != 0
		){
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

static void load_nickname_override(){
	int fd = sceIoOpen("ms0:/seplugins/nickname.txt", PSP_O_RDONLY, 0777);
	if (fd < 0){
		fd = sceIoOpen("ef0:/seplugins/nickname.txt", PSP_O_RDONLY, 0777);
	}
	if (fd >= 0){
		sceIoRead(fd, nickname, sizeof(nickname));
		nickname[sizeof(nickname) - 1] = '\0';
		for(int i = 0;i < sizeof(nickname);i++){
			if (nickname[i] == '\n' || nickname[i] == '\r'){
				nickname[i] = '\0';
			}
		}
		printk("%s: loaded nickname (%s) from storage\n", __func__, nickname);
	}
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

	load_nickname_override();

	// Enable Online Mode
	onlinemode = sceWlanGetSwitchState();
	
	// Log WLAN Switch State
	printk("WLAN Switch: %d\n", onlinemode);
	
	// Grab API Type
	// int api = sceKernelInitApitype();

	for (int i = 0;i < sizeof(fd_path_map)/sizeof(fd_path_map[0]);i++){
		fd_path_map[i].fd = -1;
	}

	// Game Mode & WLAN Switch On
	if(sceKernelInitKeyConfig() == PSP_INIT_KEYCONFIG_GAME) {
	// if(api == 0x120 || api == 0x123 || api == 0x125) {
		if (onlinemode){
			memlayout_hack();
		}

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
						if(!is_vita()){
							// Disable Sleep Mode
							scePowerLock(0);
							printk("Disabled Power Button!\n");

							#if 0
							// Monitor/disable volatile lock
							HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelVolatileMemLock), sceKernelVolatileMemLockPatched, sceKernelVolatileMemLockOrig);
							HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelVolatileMemTryLock), sceKernelVolatileMemTryLockPatched, sceKernelVolatileMemTryLockOrig);
							HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelVolatileMemUnlock), sceKernelVolatileMemUnlockPatched, sceKernelVolatileMemUnlockOrig);

							// Monitor power locks
							//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerLock), sceKernelPowerLockPatched, sceKernelPowerLockOrig);
							//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerUnlock), sceKernelPowerUnlockPatched, sceKernelPowerUnlockOrig);
							HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerLockForUser), sceKernelPowerLockForUserPatched, sceKernelPowerLockForUserOrig);
							HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceKernelPowerUnlockForUser), sceKernelPowerUnlockForUserPatched, sceKernelPowerUnlockForUserOrig);
							#endif
						}

						// Monitor netconf init
						void *netconf_init_func = (void *)sctrlHENFindFunction("sceUtility_Driver", "sceUtility", 0x4DB1E739);
						HIJACK_FUNCTION(netconf_init_func, netconf_init, netconf_init_orig);
						void *netconf_get_status_func = (void *)sctrlHENFindFunction("sceUtility_Driver", "sceUtility", 0x6332AA39);
						HIJACK_FUNCTION(netconf_get_status_func, netconf_get_status, netconf_get_status_orig);
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
