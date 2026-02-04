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

#include "../../common.h"

static int ptp_close_postoffice(int idx){
	AdhocSocket *internal = _sockets[idx];

	// stop others from trying to use it while we yield later during cleanup
	sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
	_sockets[idx] = NULL;
	sceKernelSignalSema(_socket_mapper_mutex, 1);

	if (internal->connect_thread >= 0){
		sceKernelWaitThreadEnd(internal->connect_thread, NULL);
		sceKernelDeleteThread(internal->connect_thread);
		internal->connect_thread = -1;
	}

	void *socket = internal->postoffice_handle;
	if (socket != NULL){
		if (internal->ptp.state == PTP_STATE_LISTEN){
			ptp_listen_close(socket);
		}else{
			ptp_close(socket);
		}
	}
	// postoffice should have aborted other operations at this point, if the other side yielded to us during data operations

	free(internal);
	return 0;
}

/**
 * Adhoc Emulator PTP Socket Closer
 * @param id Socket File Descriptor
 * @param flag Bitflags (Unused)
 * @return 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_INVALID_SOCKET_ID, ADHOC_SOCKET_DELETED
 */
int proNetAdhocPtpClose(int id, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Arguments & Atleast one Socket
		if(id > 0 && id <= 255 && _sockets[id - 1] != NULL && _sockets[id - 1]->is_ptp)
		{
			if (_postoffice){
				return ptp_close_postoffice(id - 1);
			}

			// Cast Socket
			SceNetAdhocPtpStat * socket = &_sockets[id - 1]->ptp;
			
			// Close Connection
			sceNetInetClose(socket->id);
			
			// Remove Port Forward from Router only if the connection is not from accept calls
			if (_sockets[id - 1]->ptp_ext.mode != PTP_MODE_ACCEPT)
				sceNetPortClose("TCP", socket->lport);
			
			// Free Memory
			free(_sockets[id - 1]);
			
			// Free Reference
			sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
			_sockets[id - 1] = NULL;
			sceKernelSignalSema(_socket_mapper_mutex, 1);
			
			// Success
			return 0;
		}
		
		// Invalid Argument
		return ADHOC_INVALID_SOCKET_ID;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}
