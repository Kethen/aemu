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
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocctlJoin(const SceNetAdhocctlScanInfo * scan_info)
{
	create_adhocctl_name_buf(group_name_buf, scan_info->group_name.data);
	#ifdef TRACE
	printk("Entering %s, group name %s\n", __func__, group_name_buf);
	#endif
	int result = proNetAdhocctlJoin(scan_info);
	//#ifdef TRACE
	printk("Leaving %s with %08X, group name %s\n", __func__, result, group_name_buf);
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
	//#ifdef TRACE
	printk("Leaving %s with %08X tid 0x%x\n", __func__, result, sceKernelGetThreadId());
	//#endif
	return result;
}

int sceNetAdhocctlConnect(const SceNetAdhocctlGroupName * group_name)
{
	create_adhocctl_name_buf(group_name_buf, group_name->data);
	#ifdef TRACE
	printk("Entering %s, group name %s\n", __func__, group_name_buf);
	#endif
	int result = proNetAdhocctlConnect(group_name);
	//#ifdef TRACE
	printk("Leaving %s with %08X, group name %s\n", __func__, result, group_name_buf);
	//#endif
	return result;
}

int sceNetAdhocctlCreate(const SceNetAdhocctlGroupName * group_name)
{
	create_adhocctl_name_buf(group_name_buf, group_name->data);
	#ifdef TRACE
	printk("Entering %s, group name %s\n", __func__, group_name);
	#endif
	int result = proNetAdhocctlConnect(group_name);
	//#ifdef TRACE
	printk("Leaving %s with %08X, group name %s\n", __func__, result, group_name_buf);
	//#endif
	return result;
}

int sceNetAdhocctlCreateEnterGameMode(const SceNetAdhocctlGroupName * group_name, int game_type, int num, const SceNetEtherAddr * members, uint32_t timeout, int flag)
{
	create_adhocctl_name_buf(group_name_buf, group_name->data);
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlCreateEnterGameMode(group_name, game_type, num, members, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: creating gamemode %s with %d members, 0x%x/%d\n", __func__, group_name_buf, num, result, result);
	return result;
}

int sceNetAdhocctlCreateEnterGameModeMin(const SceNetAdhocctlGroupName * group_name, int game_type, int min_members, int num_members, const struct SceNetEtherAddr * members, uint32_t timeout, int flag) // B0B80E80
{
	create_adhocctl_name_buf(group_name_buf, group_name->data);
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlCreateEnterGameModeMin(group_name, game_type, min_members, num_members, members, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: creating gamemode %s with %d %d members, 0x%x/%d\n", __func__, group_name_buf, min_members, num_members, result, result);
	return result;
}

int sceNetAdhocctlJoinEnterGameMode(const SceNetAdhocctlGroupName * group_name, const SceNetEtherAddr * gc, uint32_t timeout, int flag)
{
	create_adhocctl_name_buf(group_name_buf, group_name->data);
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlJoinEnterGameMode(group_name, gc, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: joinging gamemode %s, 0x%x/%d\n", __func__, group_name_buf, result, result);
	return result;
}

int sceNetAdhocctlScan(void)
{
	//#ifdef TRACE
	printk("Entering %s\n", __func__);
	//#endif
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

static const char *adhocctl_state_name(int state){
	switch(state){
		case ADHOCCTL_STATE_DISCONNECTED:
			return "ADHOCCTL_STATE_DISCONNECTED";
		case ADHOCCTL_STATE_CONNECTED:
			return "ADHOCCTL_STATE_CONNECTED";
		case ADHOCCTL_STATE_SCANNING:
			return "ADHOCCTL_STATE_SCANNING";
		case ADHOCCTL_STATE_GAMEMODE:
			return "ADHOCCTL_STATE_GAMEMODE";
	};
	return "unknown";
}

int sceNetAdhocctlGetState(int * state)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocctlGetState(state);
	#ifdef TRACE
	printk("Leaving %s with %08x %s\n", __func__, result, adhocctl_state_name(*state));
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
	#if 0
	if (info != NULL)
	{
		printk("%s: returning %d members, 0x%x/%d\n", __func__, info->num, result, result);
	}
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
	#ifdef TRACE
	printk("Leaving %s with %08X\n", "sceUtilityNetconfGetStatus", result);
	#endif
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
int rehook_inet();

static int find_hotspot_config_id(const char *ssid){
	// Counter
	int i = 0;

	// Find Hotspot by SSID
	for(; i <= 10; i++)
	{
		// Parameter Container
		netData entry;

		// Acquire SSID for Configuration
		if(sceUtilityGetNetParam(i, PSP_NETPARAM_SSID, &entry) == 0)
		{
			// Log Parameter
			printk("Reading PSP Infrastructure Profile for %s\n", entry.asString);

			// Hotspot Configuration found
			if(strcmp(entry.asString, ssid) == 0){
				printk("%s: hotspot with ssid %s found, %d\n", __func__, ssid, i);
				return i;
			}
		}
	}

	printk("%s: hostspot with ssid %s not found, using config 0\n", __func__, ssid);
	return 0;

}

static int read_hotspot_config(char *ssid_out, int ssid_out_size){
	// Open Configuration File
	int fd = sceIoOpen("ms0:/seplugins/hotspot.txt", PSP_O_RDONLY, 0777);

	// Opened Configuration File
	if(fd >= 0)
	{
		// Read Line
		_readLine(fd, ssid_out, ssid_out_size);

		return 0;
	}

	// Generic Error
	return -1;

}

void load_inet_modules();

int sceNetInit(int poolsize, int calloutprio, int calloutstack, int netintrprio, int netintrstack);

int _wifi_connected = 0;
int stop_wifi_connect_thread = 0;
static int wifi_connect_thread_func(SceSize args, void *argp){
	char ssid[128] = {0};
	int hotspot = 0;
	int read_result = read_hotspot_config(ssid, sizeof(ssid));
	if (read_result == 0){
		hotspot = find_hotspot_config_id(ssid);
	}

	load_inet_modules();
	int net_init_result = sceNetInit(0x20000, 0x20, 0, 0x20, 0);
	if (net_init_result < 0){
		printk("%s: failed initializing networking, 0x%x\n", __func__, net_init_result);
	}

	while(!stop_wifi_connect_thread){
		// watch over the connection
		while(!stop_wifi_connect_thread){
			int watchdog;
			int state_get_status = sceNetApctlGetState(&watchdog);
			if (state_get_status != 0){
				printk("%s: failed getting wifi state, initiating connect, 0x%x\n", __func__, state_get_status);
				_wifi_connected = 0;
				break;
			}

			if (watchdog == 4){
				// it is still connected
				sceKernelDelayThread(5000000);
				continue;
			}

			printk("%s: wifi state became %d, initiating connect\n", __func__, watchdog);
			_wifi_connected = 0;
			break;
		}

		while(!stop_wifi_connect_thread){
			int wlan_switch_state = sceWlanGetSwitchState();
			if (wlan_switch_state != 1){
				printk("%s: wlan switch is off... and this module is loaded... nice\n", __func__);
				sceKernelDelayThread(1000000);
				continue;
			}

			sceNetApctlTerm();
			int apctl_init_status = sceNetApctlInit(0x2000, 0x30);
			if (apctl_init_status != 0){
				printk("%s: sceNetApctlInit failed, 0x%x\n", __func__, apctl_init_status);
				sceKernelDelayThread(1000000);
				continue;
			}

			int apctl_connect_status = sceNetApctlConnect(hotspot);
			if (apctl_connect_status != 0){
				printk("%s: sceNetApctlConnect failed, 0x%x\n", __func__, apctl_connect_status);
				apctl_disconnect_and_wait_till_disconnected();
				sceKernelDelayThread(1000000);
				continue;
			}

			// Wait for Connection
			int statebefore = 0;
			int state = 0;
			while(state != 4 && !stop_wifi_connect_thread){
				// Query State
				int getstate = sceNetApctlGetState(&state);
				
				// Log State Change
				if(statebefore != state) printk("%s: new connection state: %d\n", __func__, state);

				// Query Success
				if(getstate == 0 && state != 4)
				{
					// Wait for Retry
					sceKernelDelayThread(100000);
				}

				// Query Error
				else
				{
					printk("%s: sceNetApctlGetState returned 0x%x\n", __func__, getstate);
					break;
				}

				if (state == 0 && statebefore != 0){
					printk("%s: sceNetApctlGetState got disconnect state\n", __func__);
					break;
				}

				// Save Before State
				statebefore = state;
			}

			if (state != 4){
				printk("%s: failed connecting to ap, state %d\n", __func__, state);
				apctl_disconnect_and_wait_till_disconnected();
				sceKernelDelayThread(1000000);
				continue;
			}

			break;
		}

		_wifi_connected = 1;
	}

	return 0;
}

SceUID wifi_connect_thread = -1;

static struct SceKernelThreadOptParam thread_px_stack_opt = {
	.size = sizeof(struct SceKernelThreadOptParam),
	.stackMpid = 5,
};

int partition_to_use();

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");

	rehook_inet();

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

	// TODO: cleanup this thread on module stop, if we ever get around to look into why unloading adhoc crashes most games when inet is also loaded
	thread_px_stack_opt.stackMpid = partition_to_use();
	wifi_connect_thread = sceKernelCreateThread("wifi_connect", wifi_connect_thread_func, 63, 8192, 0, &thread_px_stack_opt);
	if (wifi_connect_thread < 0){
		printk("%s: failed creating wifi connection thread, this is bad, 0x%x\n", __func__, wifi_connect_thread);
	}else{
		sceKernelStartThread(wifi_connect_thread, 0, NULL);
	}

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

	// TODO: cleanup wifi connection thread

	return 0;
}

