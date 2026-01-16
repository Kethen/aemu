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
#include <stdlib.h>
#include "library/common.h"

#define USE_WORKER_COMMON 1
#define USE_WORKER_PDP 1
#define USE_WORKER_PTP 1
#define USE_WORKER_GAMEMODE 1

#define MODULENAME "sceNetAdhoc_Library"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER + 6, 1, 4);
PSP_HEAP_SIZE_KB(500);
bool use_worker = true;

int _port_offset = 0;

SceUID _socket_mapper_mutex = -1;
SceUID _server_resolve_mutex = -1;

static uint16_t reverse_port(uint16_t port)
{
	return port - _port_offset;
}

static uint16_t offset_port(uint16_t port)
{
	// follow ppsspp
	if (port == 0)
	{
		return 0;
	}
	port += _port_offset;
	if (port == 0)
	{
		return 65535;
	}
	return port;
}

void _readPortOffsetConfig(void)
{
	// Line Buffer
	static char line[128];

	// Open Configuration File
	int fd = sceIoOpen("ms0:/seplugins/port_offset.txt", PSP_O_RDONLY, 0777);
	if (fd < 0){
		// PPSSPP style
		fd = sceIoOpen("ms0:/PSP/PLUGINS/atpro/port_offset.txt", PSP_O_RDONLY, 0777);
	}

	// Opened Configuration File
	if(fd >= 0)
	{
		char read_buf[1024] = {0};
		sceIoRead(fd, read_buf, sizeof(read_buf) - 1);

		_port_offset = atoi(read_buf);

		// Close Configuration File
		sceIoClose(fd);
	}
}


// Stubs (Optimizer converts those to Jumps)
int sceNetAdhocInit(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = 0;
	if (use_worker && USE_WORKER_COMMON){
		result = work_using_worker(INIT, 0, NULL);
	}else{
		result = proNetAdhocInit();
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif

	return result;
}

int sceNetAdhocTerm(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = 0;
	if (use_worker && USE_WORKER_COMMON){
		result = work_using_worker(TERM, 0, NULL);
	}else{
		result = proNetAdhocTerm();
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPollSocket(SceNetAdhocPollSd * sds, int nsds, uint32_t timeout, int flags)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)sds, *(uint32_t*)&nsds, timeout, *(uint32_t*)&flags};
	int result = 0;
	if (use_worker && USE_WORKER_COMMON){
		result = work_using_worker(POLL, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPollSocket(sds, nsds, timeout, flags);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocSetSocketAlert(int id, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t *)&id, *(uint32_t *)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_COMMON){
		result = work_using_worker(SET_SOCKET_ALERT, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocSetSocketAlert(id, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGetSocketAlert(int id, int * flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)flag};
	int result = 0;
	if (use_worker && USE_WORKER_COMMON){
		result = work_using_worker(GET_SOCKET_ALERT, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGetSocketAlert(id, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPdpCreate(const SceNetEtherAddr * saddr, uint16_t sport, int bufsize, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)saddr, (uint32_t)offset_port(sport), *(uint32_t*)&bufsize, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PDP){
		result = work_using_worker(PDP_CREATE, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPdpCreate(saddr, offset_port(sport), bufsize, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: pdp creation on port %u, 0x%x/%d\n", __func__, sport, result, result);
	return result;
}

int sceNetAdhocPdpSend(int id, const SceNetEtherAddr * daddr, uint16_t dport, const void * data, int len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)daddr, (uint32_t)offset_port(dport), (uint32_t)data, *(uint32_t*)&len, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PDP){
		result = work_using_worker(PDP_SEND, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPdpSend(id, daddr, offset_port(dport), data, len, timeout, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	//printk("%s: sent pdp on socket %d port %u, 0x%x\n", __func__, id, dport, result);
	return result;
}

int sceNetAdhocPdpDelete(int id, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PDP){
		result = work_using_worker(PDP_DELETE, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPdpDelete(id, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: pdp deletion %d, 0x%x/%d\n", __func__, id, result, result);
	return result;
}

int sceNetAdhocPdpRecv(int id, SceNetEtherAddr * saddr, uint16_t * sport, void * buf, int * len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	SceNetEtherAddr saddr_capture;
	uint16_t sport_capture;
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)&saddr_capture, (uint32_t)&sport_capture, (uint32_t)buf, (uint32_t)len, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PDP){
		result = work_using_worker(PDP_RECV, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPdpRecv(id, &saddr_capture, &sport_capture, buf, len, timeout, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	if (sport != NULL){
		*sport = reverse_port(sport_capture);
	}
	if (saddr != NULL){
		*saddr = saddr_capture;
	}
	//printk("%s: received pdp on socket %d port %u, 0x%x\n", __func__, id, *sport, result);
	return result;
}

int sceNetAdhoc_67346A2A(void)
{
	// WTF is this thing?
	THROW_UNIMPLEMENTED(__func__);
	return 0;
}

int sceNetAdhocGetPdpStat(int * buflen, SceNetAdhocPdpStat * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)buflen, (uint32_t)buf};
	int result = 0;
	if (use_worker && USE_WORKER_PDP){
		result = work_using_worker(PDP_GETSTAT, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGetPdpStat(buflen, buf);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpOpen(const SceNetEtherAddr * saddr, uint16_t sport, const SceNetEtherAddr * daddr, uint16_t dport, uint32_t bufsize, uint32_t rexmt_int, int rexmt_cnt, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)saddr, (uint32_t)offset_port(sport), (uint32_t)daddr, (uint32_t)offset_port(dport), bufsize, rexmt_int, *(uint32_t*)&rexmt_cnt, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_OPEN, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpOpen(saddr, offset_port(sport), daddr, offset_port(dport), bufsize, rexmt_int, rexmt_cnt, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: ptp open from %u to %u, 0x%x/%d\n", __func__, sport, dport, result, result);
	return result;
}

int sceNetAdhocPtpConnect(int id, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_CONNECT, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpConnect(id, timeout, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: ptp connect on %d, 0x%x/%d\n", __func__, id, result, result);
	return result;
}

int sceNetAdhocPtpListen(const SceNetEtherAddr * saddr, uint16_t sport, uint32_t bufsize, uint32_t rexmt_int, int rexmt_cnt, int backlog, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)saddr, (uint32_t)offset_port(sport), bufsize, rexmt_int, *(uint32_t*)&rexmt_cnt, *(uint32_t*)&backlog, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_LISTEN, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpListen(saddr, offset_port(sport), bufsize, rexmt_int, rexmt_cnt, backlog, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: ptp listen port %u, 0x%x/%d\n", __func__, sport, result, result);
	return result;
}

int sceNetAdhocPtpAccept(int id, SceNetEtherAddr * addr, uint16_t * port, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif

	SceNetEtherAddr addr_capture;
	uint16_t port_capture;
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)&addr_capture, (uint32_t)&port_capture, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_ACCEPT, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpAccept(id, &addr_capture, &port_capture, timeout, flag);
	}
	if (addr != NULL)
	{
		*addr = addr_capture;
	}
	if (port != NULL)
	{
		*port = reverse_port(port_capture);
	}

	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: ptp accept on %d, port %u, 0x%x/%d\n", __func__, id, port_capture, result, result);
	return result;
}

int sceNetAdhocPtpSend(int id, const void * data, int * len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s buf 0x%x len 0x%x %d timeout %u nonblock %d\n", __func__, data, len, *len, timeout, flag);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)data, (uint32_t)len, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_SEND, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpSend(id, data, len, timeout, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpRecv(int id, void * buf, int * len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s buf 0x%x len 0x%x %d timeout %u nonblock %d\n", __func__, buf, len, *len, timeout, flag);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)buf, (uint32_t)len, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_RECV, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpRecv(id, buf, len, timeout, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpFlush(int id, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, timeout, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_FLUSH, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpFlush(id, timeout, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpClose(int id, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, *(uint32_t*)&flag};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_CLOSE, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocPtpClose(id, flag);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: ptp close %d, 0x%x/%d\n", __func__, id, result, result);
	return result;
}

int sceNetAdhocGetPtpStat(int * buflen, SceNetAdhocPtpStat * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)buflen, (uint32_t)buf};
	int result = 0;
	if (use_worker && USE_WORKER_PTP){
		result = work_using_worker(PTP_GETSTAT, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGetPtpStat(buflen, buf);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeCreateMaster(const void * ptr, uint32_t size)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)ptr, size};
	int result = 0;
	if (use_worker && USE_WORKER_GAMEMODE){
		result = work_using_worker(GAMEMODE_CREATE_MASTER, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGameModeCreateMaster(ptr, size);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: gamemode master creation on 0x%x with size %u, 0x%x/%d\n", __func__, ptr, size, result, result);
	return result;
}

int sceNetAdhocGameModeCreateReplica(const SceNetEtherAddr * src, void * ptr, uint32_t size)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {(uint32_t)src, (uint32_t)ptr, size};
	int result = 0;
	if (use_worker && USE_WORKER_GAMEMODE){
		result = work_using_worker(GAMEMODE_CREATE_REPLICA, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGameModeCreateReplica(src, ptr, size);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	printk("%s: gamemode replica creation on 0x%x with size %u, 0x%x/%d\n", __func__, ptr, size, result, result);
	return result;
}

int sceNetAdhocGameModeUpdateMaster(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = 0;
	if (use_worker && USE_WORKER_GAMEMODE){
		result = work_using_worker(GAMEMODE_UPDATE_MASTER, 0, NULL);
	}else{
		result = proNetAdhocGameModeUpdateMaster();
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeDeleteMaster(void)
{
	//#ifdef TRACE
	printk("Entering %s\n", __func__);
	//#endif
	int result = 0;
	if (use_worker && USE_WORKER_GAMEMODE){
		result = work_using_worker(GAMEMODE_DELETE_MASTER, 0, NULL);
	}else{
		result = proNetAdhocGameModeDeleteMaster();
	}
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

int sceNetAdhocGameModeUpdateReplica(int id, SceNetAdhocGameModeOptData * opt)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)opt};
	int result = 0;
	if (use_worker && USE_WORKER_GAMEMODE){
		result = work_using_worker(GAMEMODE_UPDATE_REPLICA, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGameModeUpdateReplica(id, opt);
	}
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeDeleteReplica(int id)
{
	//#ifdef TRACE
	printk("Entering %s\n", __func__);
	//#endif
	uint32_t args[] = {*(uint32_t*)&id};
	int result = 0;
	if (use_worker && USE_WORKER_GAMEMODE){
		result = work_using_worker(GAMEMODE_DELETE_REPLICA, sizeof(args) / sizeof(args[0]), args);
	}else{
		result = proNetAdhocGameModeDeleteReplica(id);
	}
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

void init_littlec();
void clean_littlec();
int rehook_inet();

int _is_ppsspp = 0;


static SceUID ppsspp_stolen_mem = -1;
void ppsspp_steal_memory(){
	// so that gta don't die
	if (ppsspp_stolen_mem >= 0){
		return;
	}
	ppsspp_stolen_mem = sceKernelAllocPartitionMemory(2, "ppsspp_mem_layout_manipulation", PSP_SMEM_High, 1024 * 1024 * 8, NULL);
	if (ppsspp_stolen_mem < 0){
		printk("%s: failed stealing memory for ppsspp, 0x%x\n", __func__, ppsspp_stolen_mem);
	}
}

void ppsspp_return_memory(){
	if (ppsspp_stolen_mem >= 0){
		sceKernelFreePartitionMemory(ppsspp_stolen_mem);
		ppsspp_stolen_mem = -1;
	}
}

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");

	int rehook_result = rehook_inet();
	if (rehook_result == 0){
		printk("%s: vita speedup detected, disabling workers\n", __func__);
		use_worker = false;
	}else{
		printk("%s: vita speedup not detected, enabling workers\n", __func__);
		use_worker = true;
	}

	init_littlec();
	int mutex_create_status = sceKernelCreateLwMutex(&_gamemode_lock, "adhoc_gamemode_mutex", PSP_LW_MUTEX_ATTR_RECURSIVE, 0, NULL);
	if (mutex_create_status != 0)
	{
		printk("%s: failed creating gamemode mutex, 0x%x\n", __func__, mutex_create_status);
	}

	_socket_mapper_mutex = sceKernelCreateSema("socket mapper mutex", 0, 1, 1, NULL);
	if (_socket_mapper_mutex < 0){
		printk("%s: failed creating socket mapper mutex\n", __func__);
	}

	_server_resolve_mutex = sceKernelCreateSema("server resolve mutex", 0, 1, 1, NULL);
	if (_server_resolve_mutex < 0){
		printk("%s: failed creating server resolve mutex\n", __func__);
	}

	if (use_worker){
		int num_workers = init_workers();
		printk("%s: created %d workers\n", __func__, num_workers);
	}

	_is_ppsspp = sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0;
	if (_is_ppsspp){
		ppsspp_steal_memory();
		printk("%s: ppsspp detected\n", __func__);
	}

	return 0;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	printk(MODULENAME " stop!\n");
	stop_workers();
	sceKernelDeleteLwMutex(&_gamemode_lock);
	clean_littlec();
	return 0;
}
