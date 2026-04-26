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

#include "../common.h"

void get_game_code(char *buf, int len);
static int need_delay(){
	static int cache = -1;
	if (cache != -1){
		return cache;
	}

	char name_buf[20] = {0};
	get_game_code(name_buf, sizeof(name_buf));

	printk("%s: gamecode is %s\n", __func__, name_buf);

	static const char *gamecodes[] = {
		// naruto shippunden ultimate ninja heros 3
		"ULUS10518",
		"ULES01407",
		"NPUH90078",
		"NPEH90038",
		"ULJS00236",
		"ULJS19066",
		"ULKS46229",
	};

	for(int i = 0;i < sizeof(gamecodes) / sizeof(gamecodes[0]);i++){
		if (strcmp(gamecodes[i], name_buf) == 0){
			cache = 1;
			return 1;
		}
	}

	cache = 0;
	return 0;
}

/**
 * Trigger Adhoc Network Scan
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_BUSY
 */
int proNetAdhocctlScan(void)
{
	// Library initialized
	if(_init == 1)
	{
		// Not connected
		if(_thread_status == ADHOCCTL_STATE_DISCONNECTED)
		{
			// Set Thread Status to Scanning
			_thread_status = ADHOCCTL_STATE_SCANNING;
			
			// Prepare Scan Request Packet
			uint8_t opcode = OPCODE_SCAN;

			// Send Scan Request Packet
			sceNetInetSend(_metasocket, &opcode, 1, INET_MSG_DONTWAIT);

			if (need_delay()){
				sceKernelDelayThread(250000);
			}

			// Return Success
			return 0;
		}
		
		// Library is busy
		return ADHOCCTL_BUSY;
	}
	
	// Library uninitialized
	return ADHOCCTL_NOT_INITIALIZED;
}
