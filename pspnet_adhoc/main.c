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

int port_offset = 0;

static uint16_t reverse_port(uint16_t port)
{
	return port - port_offset;
}

static uint16_t offset_destination_port(uint16_t port)
{
	return port + port_offset;
}

static uint16_t offset_source_port(uint16_t port)
{
	// follow ppsspp
	if (port == 0)
	{
		return 0;
	}
	port += port_offset;
	if (port == 0)
	{
		return 65535;
	}
	return port;
}

static void _readPortOffsetConfig(void)
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

		port_offset = atoi(read_buf);

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
	int result = proNetAdhocInit();
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif

	_readPortOffsetConfig();

	return result;
}

int sceNetAdhocTerm(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocTerm();
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
	int result = proNetAdhocPollSocket(sds, nsds, timeout, flags);
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
	int result = proNetAdhocSetSocketAlert(id, flag);
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
	int result = proNetAdhocGetSocketAlert(id, flag);
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

	int result = proNetAdhocPdpCreate(saddr, offset_source_port(sport), bufsize, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPdpSend(int id, const SceNetEtherAddr * daddr, uint16_t dport, const void * data, int len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocPdpSend(id, daddr, offset_destination_port(dport), data, len, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPdpDelete(int id, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocPdpDelete(id, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPdpRecv(int id, SceNetEtherAddr * saddr, uint16_t * sport, void * buf, int * len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocPdpRecv(id, saddr, sport, buf, len, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	*sport = reverse_port(*sport);
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
	int result = proNetAdhocGetPdpStat(buflen, buf);
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
	int result = proNetAdhocPtpOpen(saddr, offset_source_port(sport), daddr, offset_destination_port(dport), bufsize, rexmt_int, rexmt_cnt, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpConnect(int id, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocPtpConnect(id, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpListen(const SceNetEtherAddr * saddr, uint16_t sport, uint32_t bufsize, uint32_t rexmt_int, int rexmt_cnt, int backlog, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	uint16_t new_port = sport;
	int result = proNetAdhocPtpListen(saddr, offset_source_port(sport), bufsize, rexmt_int, rexmt_cnt, backlog, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpAccept(int id, SceNetEtherAddr * addr, uint16_t * port, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocPtpAccept(id, addr, port, timeout, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocPtpSend(int id, const void * data, int * len, uint32_t timeout, int flag)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocPtpSend(id, data, len, timeout, flag);
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
	int result = proNetAdhocPtpRecv(id, buf, len, timeout, flag);
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
	int result = proNetAdhocPtpFlush(id, timeout, flag);
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
	int result = proNetAdhocPtpClose(id, flag);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGetPtpStat(int * buflen, SceNetAdhocPtpStat * buf)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocGetPtpStat(buflen, buf);
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
	int result = proNetAdhocGameModeCreateMaster(ptr, size);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeCreateReplica(const SceNetEtherAddr * src, void * ptr, uint32_t size)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocGameModeCreateReplica(src, ptr, size);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeUpdateMaster(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocGameModeUpdateMaster();
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeDeleteMaster(void)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocGameModeDeleteMaster();
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeUpdateReplica(int id, SceNetAdhocGameModeOptData * opt)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocGameModeUpdateReplica(id, opt);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
}

int sceNetAdhocGameModeDeleteReplica(int id)
{
	#ifdef TRACE
	printk("Entering %s\n", __func__);
	#endif
	int result = proNetAdhocGameModeDeleteReplica(id);
	#ifdef TRACE
	printk("Leaving %s with %08X\n", __func__, result);
	#endif
	return result;
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
