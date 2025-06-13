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
#include <psputility_sysparam.h>
#include <string.h>
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
//	-1
};

void steal_memory()
{
	static const int size = 1024 * 1024 * 2;

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

// Kernel Module Loader
SceUID load_plugin(const char * path, int flags, SceKernelLMOption * option)
{
	// Online Mode Enabled
	if(onlinemode)
	{
		// Replace Adhoc Modules
		int i = 0; for(; i < MODULE_LIST_SIZE; i++) {
			// Matching Modulename
			if(strstr(path, module_names[i]) != NULL) {
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
					steal_memory();
				}
				
				// Log Hotswapping
				printk("%s: Swapping %s, UID=0x%08X\n", __func__, module_names[i], result);
				
				// Return Module UID
				return result;
			}
		}
	}
	
	// Default Action - Load Module
	return sceKernelLoadModule(path, flags, option);
}

// User Module Loader
SceUID load_plugin_alt(const char * path, int unk1, int unk2, int flags, SceKernelLMOption * option)
{
	// Thanks to CFW we can load whatever we want...
	// No need to have different loader for user & kernel modules.
	return load_plugin(path, flags, option);
}

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
	return originalcall(fd, flags, option);
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
				// Create File Path
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
	return sceIoOpen(path, flags, mode);
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
			
			// Found Userspace Module
			if(module != NULL && (module->text_addr & 0x80000000) == 0)
			{
				// Hook Imports
				hook_import_bynid((SceModule *)module, "sceUtility", 0x4DB1E739, init);
				hook_import_bynid((SceModule *)module, "sceUtility", 0x6332AA39, getstatus);
				hook_import_bynid((SceModule *)module, "sceUtility", 0x91E70E35, update);
				hook_import_bynid((SceModule *)module, "sceUtility", 0xF88155F6, shutdown);

				// Lie to the game about adhoc channel for at least Ridge Racer 2
				hook_import_bynid((SceModule *)module, "sceUtility", 0xA5DA2406, get_system_param_int);
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
	printk("%s: module start %s\n", __func__, module->modname);

	if (module->text_addr > 0x08800000 && module->text_addr < 0x08900000)
	{
		// Very likely the game itself
		printk("%s: guessing this is the game, 0x%x, %s, trying to reserve memory now\n", __func__, module->text_addr, module->modname);
		early_memory_stealing();
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
				hook_import_bynid((SceModule *)module, "ModuleMgrForUser", 0x977DE386, load_plugin);
				
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
			}
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
	return sysctrl_patcher(module);
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
			u32 nid[2] = { 0xE3CCC6EA, 0x07290699 };
			#endif

			// 6.35 NIDs
			#ifdef CONFIG_63X
			u32 nid[2] = { 0xFFB9B760, 0xAFBC3911 };
			#endif

			// 6.60 NIDs
			#ifdef CONFIG_660
			u32 nid[2] = { 0x939E4270, 0xD4EE2D26 };
			#endif

			// Patch Utility Manager Imports
			result = hook_import_bynid(utility, "ModuleMgrForKernel", nid[0], load_plugin);
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
