#include "common.h"
#include "workers.h"

#include <stdbool.h>

typedef struct {
	SceUID work_sema;
	work_type type;
	uint32_t args[14];
	int ret;
	SceUID done_sema;
	bool busy;
	SceUID thid;
} worker;

static SceUID workers_mutex = -1;
static int workers_created = 0;
static worker workers[16];
static bool workers_should_stop = false;

static int worker_thread_func(SceSize args, void *argp){
	worker *w = *(worker **)argp;
	while (true){
		// Wait for work
		sceKernelWaitSema(w->work_sema, 1, 0);

		if (workers_should_stop){
			// exit
			return 0;
		}

		switch (w->type){
			case INIT:
				w->ret = proNetAdhocInit();
				break;
			case TERM:
				w->ret = proNetAdhocTerm();
				break;
			case PDP_CREATE:
				// saddr, sport, bufsize, flag
				w->ret = proNetAdhocPdpCreate((SceNetEtherAddr *)w->args[0], (uint16_t)w->args[1], *(int32_t*)&w->args[2], *(int32_t*)&w->args[3]);
				break;
			case PDP_DELETE:
				// id, flag
				w->ret = proNetAdhocPdpDelete(*(int32_t*)&w->args[0], *(int32_t*)&w->args[1]);
				break;
			case PDP_GETSTAT:
				// buflen, buf
				w->ret = proNetAdhocGetPdpStat((int*)w->args[0], (SceNetAdhocPdpStat*)w->args[1]);
				break;
			case PDP_RECV:
				// id, saddr, sport, buf, len, timeout, flag
				w->ret = proNetAdhocPdpRecv(*(int32_t*)&w->args[0], (SceNetEtherAddr*)w->args[1], (uint16_t *)w->args[2], (void *)w->args[3], (int *)w->args[4], w->args[5], *(int32_t*)&w->args[6]);
				break;
			case PDP_SEND:
				// id, daddr, dport, data, len, timeout, flag
				w->ret = proNetAdhocPdpSend(*(int32_t*)&w->args[0], (SceNetEtherAddr*)w->args[1], (uint16_t)w->args[2], (void *)w->args[3], *(int32_t *)&w->args[4], w->args[5], *(int32_t*)&w->args[6]);
				break;
			case PTP_ACCEPT:
				// id, addr, port, timeout, flag
				w->ret = proNetAdhocPtpAccept(*(int32_t*)&w->args[0], (SceNetEtherAddr*)w->args[1], (uint16_t *)w->args[2], w->args[3], *(int32_t*)&w->args[4]);
				break;
			case PTP_CLOSE:
				// id, flag
				w->ret = proNetAdhocPtpClose(*(int32_t*)&w->args[0], *(int32_t*)&w->args[1]);
				break;
			case PTP_CONNECT:
				// id, timeout, flag
				w->ret = proNetAdhocPtpConnect(*(int32_t*)&w->args[0], w->args[1], *(int32_t*)&w->args[2]);
				break;
			case PTP_FLUSH:
				// id, timeout, flag
				w->ret = proNetAdhocPtpFlush(*(int32_t*)&w->args[0], w->args[1], *(int32_t*)&w->args[2]);
				break;
			case PTP_GETSTAT:
				// buflen, buf
				w->ret = proNetAdhocGetPtpStat((int*)w->args[0], (SceNetAdhocPtpStat*)w->args[1]);
				break;
			case PTP_LISTEN:
				// saddr, sport, bufsize, rexmt_int, rexmt_cnt, backlog, flag
				w->ret = proNetAdhocPtpListen((SceNetEtherAddr*)w->args[0], (uint16_t)w->args[1], w->args[2], w->args[3], *(int32_t*)&w->args[4], *(int32_t*)&w->args[5], *(int32_t*)&w->args[6]);
				break;
			case PTP_OPEN:
				// saddr, sport, daddr, dport, bufsize, rexmt_int, rexmt_cnt, flag
				w->ret = proNetAdhocPtpOpen((SceNetEtherAddr*)w->args[0], (uint16_t)w->args[1], (SceNetEtherAddr*)w->args[2], (uint16_t)w->args[3], w->args[4], w->args[5], *(int32_t*)&w->args[6], *(int32_t*)&w->args[7]);
				break;
			case PTP_RECV:
				// id, buf, len, timeout, flag
				w->ret = proNetAdhocPtpRecv(*(int32_t*)&w->args[0], (void*)w->args[1], (int*)w->args[2], w->args[3], *(int32_t*)&w->args[4]);
				break;
			case PTP_SEND:
				// id, data, len, timeout, flag
				w->ret = proNetAdhocPtpSend(*(int32_t*)&w->args[0], (void*)w->args[1], (int*)w->args[2], w->args[3], *(int32_t*)&w->args[4]);
				break;
			case POLL:
				// sds, nsds, timeout, flags
				w->ret = proNetAdhocPollSocket((SceNetAdhocPollSd*)w->args[0], *(int32_t*)&w->args[1], w->args[2], *(int32_t*)&w->args[3]);
				break;
			case SET_SOCKET_ALERT:
				// id, flag
				w->ret = proNetAdhocSetSocketAlert(*(int32_t*)&w->args[0], *(int32_t*)&w->args[1]);
				break;
			case GET_SOCKET_ALERT:
				// id, flag
				w->ret = proNetAdhocGetSocketAlert(*(int32_t*)&w->args[0], (int *)w->args[1]);
				break;
			case GAMEMODE_CREATE_MASTER:
				// ptr, size
				w->ret = proNetAdhocGameModeCreateMaster((void*)w->args[0], w->args[1]);
				break;
			case GAMEMODE_CREATE_REPLICA:
				// src, ptr, size
				w->ret = proNetAdhocGameModeCreateReplica((SceNetEtherAddr*)w->args[0], (void*)w->args[1], w->args[2]);
				break;
			case GAMEMODE_DELETE_MASTER:
				w->ret = proNetAdhocGameModeDeleteMaster();
				break;
			case GAMEMODE_DELETE_REPLICA:
				// id
				w->ret = proNetAdhocGameModeDeleteReplica(*(int32_t*)&w->args[0]);
				break;
			case GAMEMODE_UPDATE_MASTER:
				w->ret = proNetAdhocGameModeUpdateMaster();
				break;
			case GAMEMODE_UPDATE_REPLICA:
				// id, opt
				w->ret = proNetAdhocGameModeUpdateReplica(*(int32_t*)&w->args[0], (SceNetAdhocGameModeOptData*)w->args[1]);
				break;
			default:
				printk("%s: unknown command 0x%x, please debug this\n", __func__, w->type);
				break;
		}

		// Let the other side know that work is done
		sceKernelSignalSema(w->done_sema, 1);
	}
}

int init_workers(){
	workers_mutex = sceKernelCreateSema("adhoc workers mutex", 0, 1, 1, NULL);
	if (workers_mutex < 0){
		printk("%s: failed creating worker mutex, 0x%x\n", __func__, workers_mutex);
	}
	for (int i = 0;i < sizeof(workers) / sizeof(workers[0]);i++){
		workers[i].work_sema = sceKernelCreateSema("adhoc worker work sema", 0, 0, 1, NULL);
		if (workers[i].work_sema < 0){
			printk("%s: failed creating work sema, 0x%x\n", __func__, workers[i].work_sema);
			break;
		}
		workers[i].done_sema = sceKernelCreateSema("adhoc worker done sema", 0, 0, 1, NULL);
		if (workers[i].done_sema < 0){
			printk("%s: failed creating done sema, 0x%x\n", __func__, workers[i].done_sema);
			sceKernelDeleteSema(workers[i].work_sema);
			break;
		}
		workers[i].thid = sceKernelCreateThread("adhoc worker", worker_thread_func, 0x18, 0x4000, 0, NULL);
		if (workers[i].thid < 0){
			printk("%s: failed creating kernel thread, 0x%x\n", __func__, workers[i].thid);
			sceKernelDeleteSema(workers[i].work_sema);
			sceKernelDeleteSema(workers[i].done_sema);
			break;
		}
		worker *worker_ptr = &workers[i];
		sceKernelStartThread(workers[i].thid, sizeof(worker_ptr), &worker_ptr);
		workers_created++;
	}
	return workers_created;
}

int work_using_worker(work_type type, uint32_t num_args, uint32_t *args){
	while (true){
		// First secure a worker
		worker *w = NULL;
		sceKernelWaitSema(workers_mutex, 1, 0);
		for (int i = 0;i < workers_created;i++){
			if (!workers[i].busy){
				w = &workers[i];
				w->busy = true;
				break;
			}
		}
		sceKernelSignalSema(workers_mutex, 1);
		if (w == NULL){
			sceKernelDelayThread(1000);
			printk("%s: ran out of workers, that is not good\n", __func__);
			continue;
		}

		// Put work into the worker
		w->type = type;
		if (num_args != 0) {
			memcpy(w->args, args, sizeof(uint32_t) * num_args);
		}

		// Start work on the worker
		sceKernelSignalSema(w->work_sema, 1);

		// Wait for worker to be done
		sceKernelWaitSema(w->done_sema, 1, 0);

		// Collect result and release worker
		int ret = w->ret;
		sceKernelWaitSema(workers_mutex, 1, 0);
		w->busy = false;
		sceKernelSignalSema(workers_mutex, 1);

		// Return result
		return ret;
	}
}

void stop_workers(){
	workers_should_stop = true;
	for (int i = 0;i < workers_created;i++){
		// give the worker one more signal
		sceKernelSignalSema(workers[i].work_sema, 1);
		
		// wait till the worker thread is done
		sceKernelWaitThreadEnd(workers[i].thid, 0);

		// delete the semaphores
		sceKernelDeleteSema(workers[i].work_sema);
		sceKernelDeleteSema(workers[i].done_sema);
		sceKernelDeleteSema(workers_mutex);
	}
}