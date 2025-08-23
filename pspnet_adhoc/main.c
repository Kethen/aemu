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

#define MODULENAME "sceNetAdhoc_Library"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER + 6, 1, 4);
PSP_HEAP_SIZE_KB(100);

int _port_offset = 0;

SceUID _socket_mapper_mutex = -1;

static uint16_t reverse_port(uint16_t port)
{
	return port - _port_offset;
}

static uint16_t offset_destination_port(uint16_t port)
{
	return port + _port_offset;
}

static uint16_t offset_source_port(uint16_t port)
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
	int result = work_using_worker(INIT, 0, NULL);
	//int result = proNetAdhocInit();
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
	int result = work_using_worker(TERM, 0, NULL);
	//int result = proNetAdhocTerm();
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
	int result = work_using_worker(POLL, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPollSocket(sds, nsds, timeout, flags);
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
	int result = work_using_worker(SET_SOCKET_ALERT, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocSetSocketAlert(id, flag);
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
	int result = work_using_worker(GET_SOCKET_ALERT, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGetSocketAlert(id, flag);
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
	uint32_t args[] = {(uint32_t)saddr, (uint32_t)offset_source_port(sport), *(uint32_t*)&bufsize, *(uint32_t*)&flag};
	int result = work_using_worker(PDP_CREATE, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPdpCreate(saddr, offset_source_port(sport), bufsize, flag);
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
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)daddr, (uint32_t)offset_destination_port(dport), (uint32_t)data, *(uint32_t*)&len, timeout, *(uint32_t*)&flag};
	int result = work_using_worker(PDP_SEND, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPdpSend(id, daddr, offset_destination_port(dport), data, len, timeout, flag);
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
	int result = work_using_worker(PDP_DELETE, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPdpDelete(id, flag);
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
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)saddr, (uint32_t)sport, (uint32_t)buf, (uint32_t)len, timeout, *(uint32_t*)&flag};
	int result = work_using_worker(PDP_RECV, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPdpRecv(id, saddr, sport, buf, len, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	*sport = reverse_port(*sport);
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
	int result = work_using_worker(PDP_GETSTAT, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGetPdpStat(buflen, buf);
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
	uint32_t args[] = {(uint32_t)saddr, (uint32_t)offset_source_port(sport), (uint32_t)daddr, (uint32_t)offset_destination_port(dport), bufsize, rexmt_int, *(uint32_t*)&rexmt_cnt, *(uint32_t*)&flag};
	int result = work_using_worker(PTP_OPEN, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpOpen(saddr, offset_source_port(sport), daddr, offset_destination_port(dport), bufsize, rexmt_int, rexmt_cnt, flag);
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
	int result = work_using_worker(PTP_CONNECT, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpConnect(id, timeout, flag);
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
	uint32_t args[] = {(uint32_t)saddr, (uint32_t)offset_source_port(sport), bufsize, rexmt_int, *(uint32_t*)&rexmt_cnt, *(uint32_t*)&backlog, *(uint32_t*)&flag};
	int result = work_using_worker(PTP_LISTEN, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpListen(saddr, offset_source_port(sport), bufsize, rexmt_int, rexmt_cnt, backlog, flag);
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
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)addr, (uint32_t)port, timeout, *(uint32_t*)&flag};
	int result = work_using_worker(PTP_ACCEPT, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpAccept(id, &addr_capture, &port_capture, timeout, flag);
	if (addr != NULL)
	{
		memcpy(addr, &addr_capture, sizeof(SceNetEtherAddr));
	}
	if (port != NULL)
	{
		*port = port_capture;
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
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)data, (uint32_t)len, timeout, *(uint32_t*)&flag};
	int result = work_using_worker(PTP_SEND, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpSend(id, data, len, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpRecv(int id, void * buf, int * len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint32_t args[] = {*(uint32_t*)&id, (uint32_t)buf, (uint32_t)len, timeout, *(uint32_t*)&flag};
	int result = work_using_worker(PTP_RECV, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpRecv(id, buf, len, timeout, flag);
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
	int result = work_using_worker(PTP_FLUSH, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpFlush(id, timeout, flag);
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
	int result = work_using_worker(PTP_CLOSE, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocPtpClose(id, flag);
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
	int result = work_using_worker(PTP_GETSTAT, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGetPtpStat(buflen, buf);
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
	int result = work_using_worker(GAMEMODE_CREATE_MASTER, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGameModeCreateMaster(ptr, size);
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
	int result = work_using_worker(GAMEMODE_CREATE_REPLICA, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGameModeCreateReplica(src, ptr, size);
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
	int result = work_using_worker(GAMEMODE_UPDATE_MASTER, 0, NULL);
	//int result = proNetAdhocGameModeUpdateMaster();
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
	int result = work_using_worker(GAMEMODE_DELETE_MASTER, 0, NULL);
	//int result = proNetAdhocGameModeDeleteMaster();
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
	int result = work_using_worker(GAMEMODE_UPDATE_REPLICA, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGameModeUpdateReplica(id, opt);
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
	int result = work_using_worker(GAMEMODE_DELETE_REPLICA, sizeof(args) / sizeof(args[0]), args);
	//int result = proNetAdhocGameModeDeleteReplica(id);
	//#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	//#endif
	return result;
}

void init_littlec();
void clean_littlec();
void rehook_inet();

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");

	rehook_inet();

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

	int num_workers = init_workers();
	printk("%s: created %d workers\n", __func__, num_workers);

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
