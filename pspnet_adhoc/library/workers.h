#ifndef __WORKERS_H_
#define __WORKERS_H_

#include <stdbool.h>

typedef enum {
	INIT,
	TERM,
	PDP_CREATE,
	PDP_DELETE,
	PDP_GETSTAT,
	PDP_RECV,
	PDP_SEND,
	PTP_ACCEPT,
	PTP_CLOSE,
	PTP_CONNECT,
	PTP_FLUSH,
	PTP_GETSTAT,
	PTP_LISTEN,
	PTP_OPEN,
	PTP_RECV,
	PTP_SEND,
	POLL,
	SET_SOCKET_ALERT,
	GET_SOCKET_ALERT,
	GAMEMODE_CREATE_MASTER,
	GAMEMODE_CREATE_REPLICA,
	GAMEMODE_DELETE_MASTER,
	GAMEMODE_DELETE_REPLICA,
	GAMEMODE_UPDATE_MASTER,
	GAMEMODE_UPDATE_REPLICA,
} work_type;

int init_workers();
int work_using_worker(work_type type, uint32_t num_args, uint32_t *args);
void stop_workers();

#endif