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

// Helper Functions
static void _deleteAllGMB_Internal(SceNetAdhocGameModeBufferStat * node);

int sceNetAdhocMatchingTerm();

// I wonder if there's a more generic way to detect this quirk, or what exactly I did wrong
// In most games, apctl dhcp has issues if I don't first term sceNetInet then re-init during second connect
// Gran Turismo doesn't have that issues, instead if gets stuck during the second sceNetInetInit
void get_game_code(char *buf, int len);
static int should_term_inet(){
	if (_is_ppsspp){
		return 1;
	}

	char name_buf[20] = {0};
	get_game_code(name_buf, sizeof(name_buf));

	printk("%s: gamecode is %s\n", __func__, name_buf);

	static const char *no_term_codes[] = {
		// Gran Turismo
		"UCES01245",
		"UCUS98632",
		"UCAS40265",
		"UCJS10100"
	};

	for(int i = 0;i < sizeof(no_term_codes) / sizeof(no_term_codes[0]);i++){
		if (strcmp(no_term_codes[i], name_buf) == 0){
			return 0;
		}
	}

	return 1;
}

/**
 * Adhoc Emulator Socket Library Term-Call
 * @return 0 on success or... ADHOC_BUSY
 */
int proNetAdhocTerm(void)
{
	sceKernelLockLwMutex(&_gamemode_lock, 1, 0);
	// Library is initialized
	if(_init)
	{
		// Stop adhoc_matching
		sceNetAdhocMatchingTerm();

		// Stop adhocctl
		sceNetAdhocctlTerm();

		// Stop gamemodes
		printk("%s: deleting gamemode master\n", __func__);
		proNetAdhocGameModeDeleteMaster();
		for (int i = 0;i < sizeof(_gamemode_replicas) / sizeof(GamemodeInternal *);i++)
		{
			if (_gamemode_replicas[i] != NULL)
			{
				printk("%s: deleting gamemode replica %d\n", __func__, i + 1);
				proNetAdhocGameModeDeleteReplica(i + 1);
			}
		}

		// Delete PDP Sockets
		_deleteAllPDP();
		
		// Delete PTP Sockets
		_deleteAllPTP();
		
		// Delete Gamemode Buffer
		_deleteAllGMB();
		
		#if 0
		// Terminate Internet Library
		if (should_term_inet()){
			int inet_term_result = sceNetInetTerm();
			printk("%s: sceNetInetTerm result 0x%x\n", __func__, inet_term_result);
		}else{
			printk("%s: sceNetInetTerm disabled on this game\n", __func__);
		}
		#endif
		
		// Unload Internet Modules (Just keep it in memory... unloading crashes?!)
		// if(_manage_modules != 0) sceUtilityUnloadModule(PSP_MODULE_NET_INET);
		
		// Drop Module Management
		_manage_modules = 0;
		
		// Library shutdown
		_init = 0;

		// Occupy memory again
		//steal_memory();
	}

	sceKernelUnlockLwMutex(&_gamemode_lock, 1);

	// Success
	return 0;
}

/**
 * Closes & Deletes all PDP Sockets
 */
void _deleteAllPDP(void)
{
	// Iterate Element
	int i = 0; for(; i < 255; i++)
	{
		// Active Socket
		if(_sockets[i] != NULL && !_sockets[i]->is_ptp)
		{
			if (_postoffice){
				if (_sockets[i]->postoffice_handle != NULL){
					pdp_delete(_sockets[i]->postoffice_handle);
				}
			}else{
				// Close Socket
				sceNetInetClose(_sockets[i]->pdp.id);
			}
			
			// Free Memory
			free(_sockets[i]);
			
			// Delete Reference
			_sockets[i] = NULL;
		}
	}
}

/**
 * Closes & Deletes all PTP Sockets
 */
void _deleteAllPTP(void)
{
	// Iterate Element
	int i = 0; for(; i < 255; i++)
	{
		// Active Socket
		if(_sockets[i] != NULL && _sockets[i]->is_ptp)
		{
			if (_postoffice){
				if (_sockets[i]->connect_thread >= 0){
					sceKernelWaitThreadEnd(_sockets[i]->connect_thread, NULL);
					sceKernelDeleteThread(_sockets[i]->connect_thread);
				}
				if (_sockets[i]->postoffice_handle != NULL){
					if (_sockets[i]->ptp.state == PTP_STATE_LISTEN){
						ptp_listen_close(_sockets[i]->postoffice_handle);
					}else{
						ptp_close(_sockets[i]->postoffice_handle);
					}
				}
			}else{
				// Close Socket
				sceNetInetClose(_sockets[i]->ptp.id);
			}
			
			// Free Memory
			free(_sockets[i]);
			
			// Delete Reference
			_sockets[i] = NULL;
		}
	}
}

/**
 * Deletes all Gamemode Buffer
 */
void _deleteAllGMB(void)
{
	// Delete Recursively
	_deleteAllGMB_Internal(_gmb);
	
	// Destroy Reference
	_gmb = NULL;
}

/**
 * Recursive Deleter for Gamemode Buffer
 * @param node Current Gamemode Buffer Node
 */
static void _deleteAllGMB_Internal(SceNetAdhocGameModeBufferStat * node)
{
	// Not at the End
	if(node != NULL)
	{
		// Process Rest of the List
		_deleteAllGMB_Internal(node->next);
		
		// Delete Node
		free(node);
	}
}
