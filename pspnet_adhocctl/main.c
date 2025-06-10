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

#define MODULENAME "sceNetAdhocctl_Library"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER + 6, 1, 6);
PSP_HEAP_SIZE_KB(100);

#if 0
SceLwMutexWorkarea networking_lock;
#endif
SceLwMutexWorkarea peer_lock;
SceLwMutexWorkarea group_list_lock;

// Stubs
int sceNetAdhocctlInit(int stacksize, int prio, const SceNetAdhocctlAdhocId * adhoc_id)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlInit(stacksize, prio, adhoc_id);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlJoin(const SceNetAdhocctlScanInfo * scan_info)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlJoin(scan_info);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlGetPeerList(int * buflen, SceNetAdhocctlPeerInfo * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetPeerList(buflen, buf);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetPeerInfo(SceNetEtherAddr * addr, int size, SceNetAdhocctlPeerInfo * peer_info)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetPeerInfo(addr, size, peer_info);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetAddrByName(const SceNetAdhocctlNickname * nickname, int * buflen, SceNetAdhocctlPeerInfo * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetAddrByName(nickname, buflen, buf);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlTerm(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlTerm();
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlConnect(const SceNetAdhocctlGroupName * group_name)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlConnect(group_name);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlCreate(const SceNetAdhocctlGroupName * group_name)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlConnect(group_name);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlCreateEnterGameMode(const SceNetAdhocctlGroupName * group_name, int game_type, int num, const SceNetEtherAddr * members, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlCreateEnterGameMode(group_name, game_type, num, members, timeout, flag);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlCreateEnterGameModeMin(const SceNetAdhocctlGroupName * group_name, int game_type, int min_members, int num_members, const struct SceNetEtherAddr * members, uint32_t timeout, int flag) // B0B80E80
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlCreateEnterGameModeMin(group_name, game_type, min_members, num_members, members, timeout, flag);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlJoinEnterGameMode(const SceNetAdhocctlGroupName * group_name, const SceNetEtherAddr * gc, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlJoinEnterGameMode(group_name, gc, timeout, flag);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlScan(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlScan();
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlDisconnect(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlDisconnect();
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlExitGameMode(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlExitGameMode();
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

SceNetAdhocctlHandler orighandler = NULL;

void loghandler(int event, int error, void * arg)
{
	printk("CTL: %d - %08X - %08X\n", event, error, (uint32_t)arg);
	orighandler(event, error, arg);
}

int sceNetAdhocctlAddHandler(SceNetAdhocctlHandler handler, void * arg)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	orighandler = handler;
	int result = proNetAdhocctlAddHandler(/*loghandler*/handler, arg);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlDelHandler(int id)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlDelHandler(id);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetState(int * state)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetState(state);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetAdhocId(SceNetAdhocctlAdhocId * adhoc_id)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetAdhocId(adhoc_id);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetNameByAddr(const SceNetEtherAddr * addr, SceNetAdhocctlNickname * nickname)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetNameByAddr(addr, nickname);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetParameter(SceNetAdhocctlParameter * parameter)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetParameter(parameter);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocctlGetScanInfo(int * buflen, SceNetAdhocctlScanInfo * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetScanInfo(buflen, buf);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlGetGameModeInfo(SceNetAdhocctlGameModeInfo * info)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetGameModeInfo(info);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

// Kernel Permission Stubs (Handle with extreme care!)
int sceUtilityNetconfInitStartKernel(SceUtilityNetconfParam * param)
{
	#ifdef TRACE
	printk("Entering %s\n", "sceUtilityNetconfInitStart");
	#endif
	int result = proUtilityNetconfInitStart(param);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", "sceUtilityNetconfInitStart", result);
	//#endif
	return result;
}

int sceUtilityNetconfGetStatusKernel(void)
{
	#ifdef TRACE
	printk("Entering %s\n", "sceUtilityNetconfGetStatus");
	#endif
	int result = proUtilityNetconfGetStatus();
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", "sceUtilityNetconfGetStatus", result);
	//#endif
	return result;
}

int sceUtilityNetconfUpdateKernel(int speed)
{
	#ifdef TRACE
	printk("Entering %s\n", "sceUtilityNetconfUpdate");
	#endif
	int result = proUtilityNetconfUpdate(speed);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", "sceUtilityNetconfUpdate", result);
	//#endif
	return result;
}

int sceUtilityNetconfShutdownStartKernel(void)
{
	#ifdef TRACE
	printk("Entering %s\n", "sceUtilityNetconfShutdownStart");
	#endif
	int result = proUtilityNetconfShutdownStart();
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", "sceUtilityNetconfShutdownStart", result);
	//#endif
	return result;
}

void init_littlec();
void clean_littlec();

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");
	int create_status = sceKernelCreateLwMutex(&peer_lock, "adhocctl_peer", PSP_LW_MUTEX_ATTR_RECURSIVE, 0, NULL);
	if (create_status != 0)
	{
		printk("%s: failed creating peer lock\n", __func__);
	}
	create_status = sceKernelCreateLwMutex(&group_list_lock, "adhocctl_group_list", PSP_LW_MUTEX_ATTR_RECURSIVE, 0, NULL);
	if (create_status != 0)
	{
		printk("%s: failed creating group list lock\n", __func__);
	}
	#if 0
	create_status = sceKernelCreateLwMutex(&networking_lock, "adhocctl_networking", PSP_LW_MUTEX_ATTR_RECURSIVE, 0, NULL);
	if (create_status != 0)
	{
		printk("%s: failed creating networking lock\n", __func__);
	}
	#endif
	patch_netconf_utility(sceUtilityNetconfInitStartKernel, sceUtilityNetconfGetStatusKernel, sceUtilityNetconfUpdateKernel, sceUtilityNetconfShutdownStartKernel);
	init_littlec();
	return 0;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	printk(MODULENAME " stop!\n");
	clean_littlec();
	#if 0
	sceKernelDeleteLwMutex(&networking_lock);
	#endif
	sceKernelDeleteLwMutex(&peer_lock);
	sceKernelDeleteLwMutex(&group_list_lock);
	return 0;
}

