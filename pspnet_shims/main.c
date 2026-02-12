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
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>

#include <string.h>

#include "../atpro/logs.h"

#define MODULENAME "pspnet_shims"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER + 6, 1, 6);
PSP_HEAP_SIZE_KB(HEAP_SIZE);

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a)
#define GET_JUMP_TARGET(x) (((x) & 0x03FFFFFF) << 2)
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
	pspSdkEnableInterrupts(_interrupts); \
	printk("original instructions: 0x%lx 0x%lx\n", _lw((u32)patch_buffer), _lw((u32)patch_buffer + 8)); \
}

static void apctl_event_spy(int prev_state, int new_state, int event, int error_code, void *arg){
	printk("%s: 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__, prev_state, new_state, event, error_code, arg);
}

int (*sceNetApctlInit_orig)(int stack_size, int init_priority) = NULL;
int sceNetApctlInit_patched(int stack_size, int init_priority){
	// some event handlers are... very big
	// considering we also force load fw apctl, we need to make this big as well
	if (stack_size < 0x2000){
		stack_size = 0x2000;
		printk("%s: stack size too small, uping stack size to %d\n", __func__, stack_size);
	}

	int result = sceNetApctlInit_orig(stack_size, init_priority);

	printk("%s: called sceNetApctlInit with 0x%x %d, 0x%x/%d\n", __func__, stack_size, init_priority, result, result);

	#if 0
	if (result >= 0){
		sceNetApctlAddHandler(apctl_event_spy, NULL);
	}
	#endif

	return result;
}

static int (*sceNetApctlGetInfo_orig)(int code, union SceNetApctlInfo *pInfo) = NULL;
static int sceNetApctlGetInfo_patched(int code, union SceNetApctlInfo *pInfo){
	int result = sceNetApctlGetInfo_orig(code, pInfo);

	printk("%s: called sceNetApctlGetInfo 0x%x 0x%x, 0x%x\n", __func__, code, pInfo, result);

	return result;
}

static int (*sceNetApctlAddHandler_orig)(sceNetApctlHandler handler, void *pArg);
static int sceNetApctlAddHandler_patched(sceNetApctlHandler handler, void *pArg){
	int result = sceNetApctlAddHandler_orig(handler, pArg);
	printk("%s: adding handler 0x%x 0x%x, 0x%x\n", __func__, handler, pArg, result);
	return result;
}

void hijack_sceNetApctlInit(){
	HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t*)sceNetApctlInit), sceNetApctlInit_patched, sceNetApctlInit_orig);
	//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t*)sceNetApctlGetInfo), sceNetApctlGetInfo_patched, sceNetApctlGetInfo_orig);
	//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t*)sceNetApctlAddHandler), sceNetApctlAddHandler_patched, sceNetApctlAddHandler_orig);
}

int initialized = 0;
int (*sceNetResolverInit_orig)() = NULL;
int sceNetResolverInit_patched(){
	if (initialized){
		printk("%s: blocked re-initialization\n", __func__);
		return 0;
	}

	int result = sceNetResolverInit_orig();
	printk("%s: called sceNetResolverInit, 0x%x\n", __func__, result);

	initialized = result == 0;

	return result;
}

int (*sceNetResolverTerm_orig)() = NULL;
int sceNetResolverTerm_patched(){
	printk("%s: blocked termination\n", __func__);
	return 0;
}

int (*sceNetResolverCreate_orig)(int *rid, void *buf, SceSize buflen) = NULL;
int sceNetResolverCreate_patched(int *rid, void *buf, SceSize buflen){
	int result = sceNetResolverCreate_orig(rid, buf, buflen);
	printk("%s: called sceNetResolverCreate with 0x%x 0x%x %u, 0x%x\n", __func__, rid, buf, buflen, result);
	return result;
}

void hijack_sceNetResolver(){
	initialized = 0;
	HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceNetResolverInit), sceNetResolverInit_patched, sceNetResolverInit_orig);
	HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceNetResolverTerm), sceNetResolverTerm_patched, sceNetResolverTerm_orig);
	//HIJACK_FUNCTION(GET_JUMP_TARGET(*(uint32_t *)sceNetResolverCreate), sceNetResolverCreate_patched, sceNetResolverCreate_orig);
}

void init_littlec();
void clean_littlec();

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");
	init_littlec();

	return 0;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	printk(MODULENAME " stop!\n");
	clean_littlec();

	return 0;
}
