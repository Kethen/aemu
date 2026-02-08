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
#include "library/common.h"

#define MODULENAME "sceNetAdhocMatching_Library"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER + 6, 1, 6);
PSP_HEAP_SIZE_KB(HEAP_SIZE);

// Original Matching Handler
SceNetAdhocMatchingHandler originalHandler = NULL;

SceLwMutexWorkarea context_list_lock;
SceLwMutexWorkarea members_lock;

// Replacement Matching Handler
void replacementHandler(int id, int event, SceNetEtherAddr * peer, int optlen, void * opt)
{
	// Log Event
	printk("%d - %d - ", id, event);
	printk("%02X:%02X:%02X:%02X:%02X:%02X", peer->data[0], peer->data[1], peer->data[2], peer->data[3], peer->data[4], peer->data[5]);
	printk(" - %d - %08X\n", optlen, (uint32_t)opt);
	
	// Forward Event
	return originalHandler(id, event, peer, optlen, opt);
}

void init_littlec();
void clean_littlec();

// Stubs
int sceNetAdhocMatchingInit(uint32_t poolsize)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	init_littlec();
	int result = proNetAdhocMatchingInit(poolsize);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingTerm(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingTerm();
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingCreate(int mode, int maxnum, uint16_t port, int rxbuflen, uint32_t hello_int, uint32_t keepalive_int, int init_count, uint32_t rexmt_int, SceNetAdhocMatchingHandler handler)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	originalHandler = handler;
	int result = proNetAdhocMatchingCreate(mode, maxnum, port, rxbuflen, hello_int, keepalive_int, init_count, rexmt_int, handler/*replacementHandler*/);
	//#ifdef TRACE
	printk("Leaving %s maxnum %d port %u rxbuflen %d hello_int %u keepalive_int %u init_count %d rexmt_int %u handler 0x%x with %08X\n", __func__, maxnum, port, rxbuflen, hello_int, keepalive_int, init_count, rexmt_int, handler, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingStart(int id, int event_th_prio, int event_th_stack, int input_th_prio, int input_th_stack, int hello_optlen, const void * hello_opt)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingStart(id, event_th_prio, event_th_stack, input_th_prio, input_th_stack, hello_optlen, hello_opt);
	//#ifdef TRACE
	printk("Leaving %s hello_optlen %d hello_opt 0x%x with %08X\n", __func__, hello_optlen, hello_opt, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingStop(int id)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingStop(id);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingDelete(int id)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingDelete(id);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingSelectTarget(int id, const SceNetEtherAddr * target, int optlen, const void * opt)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingSelectTarget(id, target, optlen, opt);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingCancelTarget(int id, const SceNetEtherAddr * target)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingCancelTargetWithOpt(id, target, 0, NULL);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingCancelTargetWithOpt(int id, const SceNetEtherAddr * target, int optlen, const void * opt)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingCancelTargetWithOpt(id, target, optlen, opt);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingSendData(int id, const SceNetEtherAddr * target, int datalen, const void * data)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingSendData(id, target, datalen, data);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingAbortSendData(int id, const SceNetEtherAddr * target)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingAbortSendData(id, target);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingSetHelloOpt(int id, int optlen, const void * opt)
{
	#ifdef TRACE
	printk("Entering %s id %d optlen %d opt 0x%x\n", __func__, id, optlen, opt);
	#endif
	int result = proNetAdhocMatchingSetHelloOpt(id, optlen, opt);
	//#ifdef TRACE
	printk("Leaving %s id %d optlen %d opt 0x%x with %08X\n", __func__, id, optlen, opt, result);
	//#endif
	return result;
}

int sceNetAdhocMatchingGetHelloOpt(int id, int * buflen, void * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingGetHelloOpt(id, buflen, buf);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingGetMembers(int id, int * buflen, SceNetAdhocMatchingMember * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingGetMembers(id, buflen, buf);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingGetPoolMaxAlloc(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingGetPoolMaxAlloc();
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocMatchingGetPoolStat(SceNetMallocStat * poolstat)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocMatchingGetPoolStat(poolstat);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");
	int mutex_create_status = sceKernelCreateLwMutex(&context_list_lock, "adhoc_matching_ctx_list", PSP_LW_MUTEX_ATTR_RECURSIVE, 0, NULL);
	if (mutex_create_status != 0)
	{
		printk("%s: failed creating matching context list mutex, 0x%x\n", __func__, mutex_create_status);
	}
	mutex_create_status = sceKernelCreateLwMutex(&members_lock, "adhoc_matching_members", PSP_LW_MUTEX_ATTR_RECURSIVE, 0, NULL);
	if (mutex_create_status != 0)
	{
		printk("%s: failed creating matching members mutex, 0x%x\n", __func__, mutex_create_status);
	}

	return 0;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	printk(MODULENAME " stop!\n");
	clean_littlec();
	sceKernelDeleteLwMutex(&context_list_lock);
	sceKernelDeleteLwMutex(&members_lock);
	return 0;
}
