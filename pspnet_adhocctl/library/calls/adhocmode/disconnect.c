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

uint64_t _disconnect_timestamp = 0;

uint64_t get_last_matching_packet_send();

SceUID _disconnect_thread = -1;
static struct SceKernelThreadOptParam thread_px_stack_opt = {
	.size = sizeof(thread_px_stack_opt),
	.stackMpid = 5,
};

int disconnect_thread_func(SceSize args, void *argp){
	// Connected State (Adhoc Mode)
	if(_thread_status == ADHOCCTL_STATE_CONNECTED)
	{
		// Clear Network Name
		memset(&_parameter.group_name, 0, sizeof(_parameter.group_name));

		// Set Disconnected State
		_thread_status = ADHOCCTL_STATE_DISCONNECTED;

		// Set HUD Connection Status
		setConnectionStatus(0);

		// Prepare Packet
		uint8_t opcode = OPCODE_DISCONNECT;

		#if 1
		// grace period for matching
		uint64_t last_matching_send = get_last_matching_packet_send();
		uint64_t now = sceKernelGetSystemTimeWide();
		uint64_t diff = now - last_matching_send;
		static const uint64_t grace_period = 500000;
		if (diff < grace_period){
			uint64_t delay = grace_period - diff;
			printk("%s: holding up disconnect for %d usecs for matching packets\n", __func__, delay);
			sceKernelDelayThread(delay);
		}
		#endif

		// Acquire Network Lock
		_acquireNetworkLock();

		// Send Disconnect Request Packet
		sceNetInetSend(_metasocket, &opcode, 1, INET_MSG_DONTWAIT);

		// Free Network Lock
		_freeNetworkLock();

		// Multithreading Lock
		_acquirePeerLock();

		// Clear Peer List
		_freeFriendsRecursive(_friends);

		// Delete Peer Reference
		_friends = NULL;

		// Multithreading Unlock
		_freePeerLock();
	}

	return 0;
}

int partition_to_use();

static void busy_wait(int us){
	uint64_t begin = sceKernelGetSystemTimeWide();
	while(begin - sceKernelGetSystemTimeWide() < us){
	}
}

/**
 * Leave current Network
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_BUSY
 */
int proNetAdhocctlDisconnect(void)
{
	// Library initialized
	if(_init == 1)
	{
		// Do not delay here, Tekken 6 hates that
		if (_disconnect_thread >= 0){
			sceKernelWaitThreadEnd(_disconnect_thread, NULL);
			sceKernelDeleteThread(_disconnect_thread);
			_disconnect_thread = -1;
		}

		thread_px_stack_opt.stackMpid = partition_to_use();
		_disconnect_thread = sceKernelCreateThread("disconnect thread", disconnect_thread_func, 100, 0x3000, 0, NULL);
		if (_disconnect_thread < 0){
			printk("%s: failed creating disconnect thread, 0x%x\n", __func__, _disconnect_thread);
		}else{
			sceKernelStartThread(_disconnect_thread, 0, NULL);
		}

		// Either way it's not game mode anymore
		_num_gamemode_peers = 0;
		_in_gamemode = 0;

		// https://github.com/hrydgard/ppsspp/pull/18435/changes
		//busy_wait(16667);
		//sceKernelDelayThread(16667);

		// Notify Event Handlers (even if we weren't connected, not doing this will freeze games like God Eater, which expect this behaviour)
		//_notifyAdhocctlhandlers(ADHOCCTL_EVENT_DISCONNECT, 0);
		_disconnect_timestamp = sceKernelGetSystemTimeWide();

		// Return Success
		return 0;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}
