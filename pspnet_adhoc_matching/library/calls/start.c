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

// Prototypes
int _setupMatchingThreads(SceNetAdhocMatchingContext * context, int event_th_prio, int event_th_stack, int input_th_prio, int input_th_stack);
int _matchingEventThread(SceSize args, void * argp);
int _matchingInputThread(SceSize args, void * argp);

// Packet Handler
void _actOnPingPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac);
void _actOnHelloPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length);
void _actOnJoinPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length);
void _actOnAcceptPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length);
void _actOnCancelPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length, uint64_t delayed_removal);
void _actOnBulkDataPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length);
void _actOnBirthPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length);
void _actOnDeathPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length);
void _actOnByePacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint64_t delayed_removal);
void _postAcceptCleanPeerList(SceNetAdhocMatchingContext * context);
void _postAcceptAddSiblings(SceNetAdhocMatchingContext * context, int siblingcount, SceNetEtherAddr * siblings);

// Broadcaster
void _broadcastPingMessage(SceNetAdhocMatchingContext * context);
void _broadcastHelloMessage(SceNetAdhocMatchingContext * context);

// Timeout Handler
void _handleTimeout(SceNetAdhocMatchingContext * context);

// Packet Response Handler
uint64_t last_packet_send = 0;
void _sendAcceptPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int optlen, void * opt);
void _sendJoinPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int optlen, void * opt);
void _sendCancelPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int optlen, void * opt);
void _sendBulkDataPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int datalen, void * data);
void _sendBirthPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac);
void _sendDeathPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac);
void _sendByePacket(SceNetAdhocMatchingContext * context);

// Broadcast MAC
uint8_t _broadcast[ETHER_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#pragma GCC push_options
#pragma GCC optimize ("O0")
static int get_gp_value()
{
	register int gp asm ("gp");
	int gp_value = gp;
	return gp_value;
}

static int set_gp_value(int gp_value)
{
	register int gp asm ("gp");
	gp = gp_value;
	return 0;
}
#pragma GCC pop_options

/**
 * Start Matching Context
 * @param id Matching Context ID
 * @param event_th_prio Event Thread Priority
 * @param event_th_stack Event Thread Stack
 * @param input_th_prio IO Thread Priority
 * @param input_th_stack IO Thread Stack
 * @param hello_optlen Hello Data Length
 * @param hello_opt Hello Data
 * @return 0 on success or... ADHOC_MATCHING_NOT_INITIALIZED, ADHOC_MATCHING_STACKSIZE_TOO_SHORT, ADHOC_MATCHING_INVALID_ID, ADHOC_MATCHING_IS_RUNNING, ADHOC_MATCHING_INVALID_OPTLEN, ADHOC_MATCHING_NO_SPACE
 */
int proNetAdhocMatchingStart(int id, int event_th_prio, int event_th_stack, int input_th_prio, int input_th_stack, int hello_optlen, const void * hello_opt)
{
	sceKernelLockLwMutex(&context_list_lock, 1, 0);
	#define RETURN_UNLOCK(_v) { \
		sceKernelUnlockLwMutex(&context_list_lock, 1); \
		return _v; \
	}

	// Library initialized
	if(_init == 1)
	{
		// Find Matching Context for ID
		SceNetAdhocMatchingContext * context = _findMatchingContext(id);
		
		// Found Matching Context
		if(context != NULL)
		{
			// Context not running yet
			if(!context->running)
			{
				// Valid Hello Data Length
				if((hello_optlen == 0 && hello_opt == NULL) || (hello_optlen > 0 && hello_opt != NULL))
				{
					// Thread-Safe Hello Data
					void * safe_hello_opt = NULL;
					
					// Cloning Required
					if(hello_optlen > 0)
					{
						// Allocate Memory
						safe_hello_opt = _malloc(hello_optlen);
						
						// Clone Data
						if(safe_hello_opt != NULL) memcpy(safe_hello_opt, hello_opt, hello_optlen);
						
						// Out of Memory
						else RETURN_UNLOCK(ADHOC_MATCHING_NO_SPACE);
					}
					
					// Save Hello Data
					context->hellolen = hello_optlen;
					context->hello = safe_hello_opt;
					
					// Setup Threads
					if(_setupMatchingThreads(context, event_th_prio, event_th_stack, input_th_prio, input_th_stack) == 0)
					{
						// Set Running Bit
						context->running = 1;

						context->gp_value = get_gp_value();

						printk("%s: gp reg value 0x%x stored\n", __func__, context->gp_value);
						
						// Start Success
						RETURN_UNLOCK(0);
					}
					
					// Clean up Memory on Thread Setup Failure
					else
					{
						// Free Hello Data Buffer
						_free(context->hello);
						
						// Delete Hello Data Information
						context->hellolen = 0;
						context->hello = NULL;
					}
					
					// Out of Memory
					RETURN_UNLOCK(ADHOC_MATCHING_NO_SPACE);
				}
				
				// Invalid Hello Data Length
				RETURN_UNLOCK(ADHOC_MATCHING_INVALID_OPTLEN);
			}
			
			// Already started
			RETURN_UNLOCK(ADHOC_MATCHING_IS_RUNNING);
		}
		
		// Invalid Matching ID
		RETURN_UNLOCK(ADHOC_MATCHING_INVALID_ID);
	}
	
	// Uninitialized Library
	RETURN_UNLOCK(ADHOC_MATCHING_NOT_INITIALIZED);
}

/**
 * Setup Matching Threads for Context
 * @param context Matching Context
 * @param event_th_prio Event Caller Thread Priority
 * @param event_th_stack Event Caller Thread Stack Size
 * @param input_th_prio IO Handler Thread Priority
 * @param input_th_stack IO Handler Thread Stack Size
 * @return 0 on success or... -1
 */
int _setupMatchingThreads(SceNetAdhocMatchingContext * context, int event_th_prio, int event_th_stack, int input_th_prio, int input_th_stack)
{
	// Fix Input Thread Stack Size
	input_th_stack = 50 * 1024;
	
	#if 0
	// Thread Name Buffer
	char threadname[128];
	// Create Event Thread Name
	sprintf(threadname, "matching_ev%d", context->id);
	// Create Event Thread
	context->event_thid = sceKernelCreateThread(threadname, _matchingEventThread, event_th_prio, event_th_stack, 0, NULL);
	#else
	// Create Event Thread
	context->event_thid = sceKernelCreateThread("matching_ev", _matchingEventThread, event_th_prio, event_th_stack, 0, NULL);
	#endif
	
	// Created Event Thread
	if(context->event_thid > 0)
	{
		// Started Event Thread
		if(sceKernelStartThread(context->event_thid, sizeof(context), &context) == 0)
		{
			// Create IO Thread Name
			#if 0
			sprintf(threadname, "matching_io%d", context->id);
			// Create IO Thread
			context->input_thid = sceKernelCreateThread(threadname, _matchingInputThread, input_th_prio, input_th_stack, 0, NULL);
			#else
			context->input_thid = sceKernelCreateThread("matching_io", _matchingInputThread, input_th_prio, input_th_stack, 0, NULL);
			#endif
			
			// Created IO Thread
			if(context->input_thid > 0)
			{
				// Started Input Thread
				if(sceKernelStartThread(context->input_thid, sizeof(context), &context) == 0)
				{
					// Setup Success
					return 0;
				}
				
				// Delete IO Thread
				sceKernelDeleteThread(context->input_thid);
				
				// Delete IO Thread Reference
				context->input_thid = 0;
			}
			
			// Clean Event Thread Shutdown
			context->event_thid = -1;
			
			// Wait for Shutdown
			while(context->event_thid != 0) sceKernelDelayThread(10000);
			
			// Return Generic Error
			return -1;
		}
		
		// Delete Event Thread
		sceKernelDeleteThread(context->event_thid);
		
		// Delete Event Thread Reference
		context->event_thid = 0;
	}
	
	// Return Generic Error
	return -1;
}

static const char *get_opcode_name(uint32_t opcode){
	switch(opcode)
	{
		case ADHOC_MATCHING_PACKET_PING:
			return "ping";
		case ADHOC_MATCHING_PACKET_HELLO:
			return "hello";
		case ADHOC_MATCHING_PACKET_JOIN:
			return "join";
		case ADHOC_MATCHING_PACKET_ACCEPT:
			return "accept";
		case ADHOC_MATCHING_PACKET_CANCEL:
			return "cancel";
		case ADHOC_MATCHING_PACKET_BULK:
			return "bulk";
		case ADHOC_MATCHING_PACKET_BULK_ABORT:
			return "bulk abort";
		case ADHOC_MATCHING_PACKET_BIRTH:
			return "birth";
		case ADHOC_MATCHING_PACKET_DEATH:
			return "death";
		case ADHOC_MATCHING_PACKET_BYE:
			return "bye";
	}
	return "unknown";
}

static const char *get_event_name(uint32_t event)
{
	switch(event)
	{
		case ADHOC_MATCHING_EVENT_HELLO:
			return "hello";
		case ADHOC_MATCHING_EVENT_REQUEST:
			return "request";
		case ADHOC_MATCHING_EVENT_LEAVE:
			return "leave";
		case ADHOC_MATCHING_EVENT_DENY:
			return "deny";
		case ADHOC_MATCHING_EVENT_CANCEL:
			return "cancel";
		case ADHOC_MATCHING_EVENT_ACCEPT:
			return "accept";
		case ADHOC_MATCHING_EVENT_ESTABLISHED:
			return "established";
		case ADHOC_MATCHING_EVENT_TIMEOUT:
			return "timeout";
		case ADHOC_MATCHING_EVENT_ERROR:
			return "error";
		case ADHOC_MATCHING_EVENT_BYE:
			return "bye";
		case ADHOC_MATCHING_EVENT_DATA:
			return "data";
		case ADHOC_MATCHING_EVENT_DATA_ACK:
			return "data ack";
		case ADHOC_MATCHING_EVENT_DATA_TIMEOUT:
			return "date timeout";
	}
	return "unknown";
}

/**
 * Matching Event Dispatcher Thread
 * @param args sizeof(SceNetAdhocMatchingContext *)
 * @param argp SceNetAdhocMatchingContext *
 * @return Exit Point is never reached...
 */
int _matchingEventThread(SceSize args, void * argp)
{
	// Cast Context
	SceNetAdhocMatchingContext * context = *(SceNetAdhocMatchingContext **)argp;
	
	// Run while needed...
	while(context->event_thid > 0)
	{
		// Messages on Stack ready for processing
		if(context->event_stack != NULL && context->event_stack_lock == 0)
		{
			// Claim Stack
			context->event_stack_lock = 1;
			
			// Iterate Message List
			ThreadMessage * msg = context->event_stack; for(; msg != NULL; msg = msg->next)
			{
				// Default Optional Data
				void * opt = NULL;
				
				// Grab Optional Data
				if(msg->optlen > 0) opt = ((void *)msg) + sizeof(ThreadMessage);
				
				// Log Matching Events
				printk("%s: Matching Event [ID=%d] [EVENT=%d/%s]\n", __func__, context->id, msg->opcode, get_event_name(msg->opcode));

				// align opt for safety
				void *opt_buffer = _malloc(msg->optlen + 8);
				if (opt_buffer != NULL)
				{
					memcpy(opt_buffer, &msg->mac, sizeof(SceNetEtherAddr));
					memset(opt_buffer + 6, 0, 2);
					memcpy(opt_buffer + 8, opt, msg->optlen);
				}
				else
				{
					printk("%s: failed allocating aligned opt buffer, game might have issue processing opt\n", __func__);
				}

				printk("%s: handler at 0x%x free stack 0x%x\n", __func__, context->handler, sceKernelGetThreadStackFreeSize(0));

				// Apply gp value obtained earlier, this looks a bit cursed but needed for GTA:VCS
				int old_gp_value = get_gp_value();

				// Use the gp reg pointer from the thread calling start
				set_gp_value(context->gp_value);

				// XXXXXXX Be very careful here, thread local storage is likely weird with changed gp

				if (opt_buffer != NULL)
				{
					// Call Event Handler
					context->handler(context->id, msg->opcode, opt_buffer, msg->optlen, opt_buffer + 8);
				}
				else
				{
					// Call Event Handler
					context->handler(context->id, msg->opcode, &msg->mac, msg->optlen, opt);
				}

				// Restore gp
				set_gp_value(old_gp_value);

				if (opt_buffer != NULL)
				{
					_free(opt_buffer);
				}
			}
			
			// Clear Event Message Stack
			_clearStack(context, ADHOC_MATCHING_EVENT_STACK);
			
			// Free Stack
			context->event_stack_lock = 0;
		}
		
		// Share CPU Time
		sceKernelDelayThread(10000);
	}
	
	// Process Last Messages
	if(context->event_stack != NULL)
	{
		// Wait to claim Stack
		while(context->event_stack_lock) sceKernelDelayThread(10000);
		
		// Claim Stack
		context->event_stack_lock = 1;
		
		// Iterate Message List
		ThreadMessage * msg = context->event_stack; for(; msg != NULL; msg = msg->next)
		{
			// Default Optional Data
			void * opt = NULL;
			
			// Grab Optional Data
			if(msg->optlen > 0) opt = ((void *)msg) + sizeof(ThreadMessage);
			
			// Call Event Handler
			context->handler(context->id, msg->opcode, &msg->mac, msg->optlen, opt);
		}
		
		// Clear Event Message Stack
		_clearStack(context, ADHOC_MATCHING_EVENT_STACK);
		
		// Free Stack
		context->event_stack_lock = 0;
	}
	
	// Delete Pointer Reference (and notify caller about finished cleanup)
	context->event_thid = 0;
	
	// Terminate Thread
	sceKernelExitDeleteThread(0);
	
	// Return Zero to shut up Compiler (never reached anyway)
	return 0;
}

static uint64_t get_time_with_delay(uint64_t delay)
{
	return sceKernelGetSystemTimeWide() + delay;
}

static int timeout_missing_peers_on_adhocctl(SceNetAdhocMatchingContext *context)
{
	sceKernelLockLwMutex(&members_lock, 1, 0);
	SceNetAdhocMatchingMemberInternal * item = context->peerlist; for(; item != NULL; item = item->next)
	{
		// Self
		if(item->state == 0)
		{
			continue;
		}
		// XXX oh no
		uint8_t peer[sizeof(struct SceNetAdhocctlPeerInfo) + 4];
		int get_peer_info_status = sceNetAdhocctlGetPeerInfo((void *)&item->mac, sizeof(peer), (void *)peer);
		if (get_peer_info_status != 0)
		{
			printk("%s: peer %02x:%02x:%02x:%02x:%02x:%02x missing on adhocctl\n", __func__, (uint32_t)item->mac.data[0], (uint32_t)item->mac.data[1], (uint32_t)item->mac.data[2], (uint32_t)item->mac.data[3], (uint32_t)item->mac.data[4], (uint32_t)item->mac.data[5]);
			printk("%s: sceNetAdhocctlGetPeerInfo status 0x%x\n", __func__, get_peer_info_status);
			// 3 seconds
			if (sceKernelGetSystemTimeWide() - item->last_seen_on_adhocctl > 3000000)
			{
				//item->lastping = 0;
				// simulate a bye packet
				_actOnByePacket(context, &item->mac, 0);
			}
		}
		else
		{
			item->last_seen_on_adhocctl = sceKernelGetSystemTimeWide();
			// PPSSPP syncs adhocctl pings with lastping
			item->lastping = item->last_seen_on_adhocctl;
		}
	}
	sceKernelUnlockLwMutex(&members_lock, 1);
}

/**
 * Matching IO Handler Thread
 * @param args sizeof(SceNetAdhocMatchingContext *)
 * @param argp SceNetAdhocMatchingContext *
 * @return Exit Point is never reached...
 */
int _matchingInputThread(SceSize args, void * argp)
{
	// Cast Context
	SceNetAdhocMatchingContext * context = *(SceNetAdhocMatchingContext **)argp;
	
	// Last Ping
	uint64_t lastping = 0;
	
	// Last Hello
	uint64_t lasthello = 0;

	// Last adhocctl check
	uint64_t last_adhocctl_check = 0;

	// Run while needed...
	while(context->input_thid > 0)
	{
		// Hello Message Sending Context with unoccupied Slots
		if((context->mode == ADHOC_MATCHING_MODE_PARENT && _countChildren(context) < (context->maxpeers - 1)) || (context->mode == ADHOC_MATCHING_MODE_P2P && _findP2P(context) == NULL))
		{
			// Hello Message Broadcast necessary because of Hello Interval, PPSSPP don't send hellos when hello_int is 0
			if(context->hello_int > 0 && sceKernelGetSystemTimeWide() - lasthello >= context->hello_int)
			{
				// Broadcast Hello Message
				_broadcastHelloMessage(context);
				
				// Update Hello Timer
				lasthello = sceKernelGetSystemTimeWide();
			}
		}
		
		// Ping Required, on PPSSPP, ping is not sent when keepalive_int is 0, here we do it anyway, otherwise we get kicked by PPSSPP..........
		//if(context->keepalive_int > 0 && sceKernelGetSystemTimeWide() - lastping >= context->keepalive_int)
		if(sceKernelGetSystemTimeWide() - lastping >= 500000)
		{
			// Broadcast Ping Message
			_broadcastPingMessage(context);
			
			// Update Ping Timer
			lastping = sceKernelGetSystemTimeWide();
		}

		// Messages on Stack ready for processing
		if(context->input_stack != NULL && context->input_stack_lock == 0)
		{
			// dirty
			while(context->input_stack_reverse_lock){
				sceKernelDelayThread(1000);
			}

			// Claim Stack
			// XXX these int locks are scary, probably should convert them to lwmutex, an interrupt here and some bad luck it could go borked
			context->input_stack_lock = 1;

			// Lock member access since the list will be touched here
			sceKernelLockLwMutex(&members_lock, 1, 0);

			// Iterate Message List
			ThreadMessage * msg = context->input_stack; for(; msg != NULL; msg = msg->next)
			{
				// Default Optional Data
				void * opt = NULL;
				
				// Grab Optional Data
				if(msg->optlen > 0) opt = ((void *)msg) + sizeof(ThreadMessage);

				printk("%s: Sending packet (Opcode:%d/%s), optlen %d, opt 0x%x\n", __func__, msg->opcode, get_opcode_name(msg->opcode), msg->optlen, opt);

				// Send Accept Packet
				if(msg->opcode == ADHOC_MATCHING_PACKET_ACCEPT) _sendAcceptPacket(context, &msg->mac, msg->optlen, opt);
				
				// Send Join Packet
				else if(msg->opcode == ADHOC_MATCHING_PACKET_JOIN) _sendJoinPacket(context, &msg->mac, msg->optlen, opt);
				
				// Send Cancel Packet
				else if(msg->opcode == ADHOC_MATCHING_PACKET_CANCEL) _sendCancelPacket(context, &msg->mac, msg->optlen, opt);
				
				// Send Bulk Data Packet
				else if(msg->opcode == ADHOC_MATCHING_PACKET_BULK) _sendBulkDataPacket(context, &msg->mac, msg->optlen, opt);
				
				// Send Birth Packet
				else if(msg->opcode == ADHOC_MATCHING_PACKET_BIRTH) _sendBirthPacket(context, &msg->mac);

				// Send Death Packet
				else if (msg->opcode == ADHOC_MATCHING_PACKET_DEATH) _sendDeathPacket(context, &msg->mac);

				// Cancel Bulk Data Transfer (does nothing as of now as we fire and forget anyway)
				// else if(msg->opcode == ADHOC_MATCHING_PACKET_BULK_ABORT) blabla;
			}
			
			// Clear IO Message Stack
			_clearStack(context, ADHOC_MATCHING_INPUT_STACK);

			// Allow member access again
			sceKernelUnlockLwMutex(&members_lock, 1);

			// Free Stack
			context->input_stack_lock = 0;
		}

		// Process all available packets like PPSSPP does
		int recvresult = 0;
		int rxbuflen = 0;
		int batch_process_packet_limit = 20;
		do{
			SceNetEtherAddr sendermac;
			uint16_t senderport;
			rxbuflen = context->rxbuflen;
			recvresult = sceNetAdhocPdpRecv(context->socket, &sendermac, &senderport, context->rxbuf, &rxbuflen, 0, ADHOC_F_NONBLOCK);

			// Received Data from a Sender that interests us
			if(recvresult == 0 && rxbuflen > 0 && context->port == senderport)
			{
				// Lock member access since the list will be touched here
				sceKernelLockLwMutex(&members_lock, 1, 0);

				// Small delay before eviting peers on cancel/bye
				uint64_t delayed_removal = get_time_with_delay(120000);

				// Log Receive Success
				printk("%s: Received %d Bytes (Opcode: %d/%s)\n", __func__, rxbuflen, context->rxbuf[0], get_opcode_name(context->rxbuf[0]));

				// Ping Packet
				if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_PING) _actOnPingPacket(context, &sendermac);

				// Hello Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_HELLO) _actOnHelloPacket(context, &sendermac, rxbuflen);

				// Join Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_JOIN) _actOnJoinPacket(context, &sendermac, rxbuflen);

				// Accept Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_ACCEPT) _actOnAcceptPacket(context, &sendermac, rxbuflen);

				// Cancel Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_CANCEL) _actOnCancelPacket(context, &sendermac, rxbuflen, delayed_removal);

				// Bulk Data Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_BULK) _actOnBulkDataPacket(context, &sendermac, rxbuflen);

				// Birth Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_BIRTH) _actOnBirthPacket(context, &sendermac, rxbuflen);

				// Death Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_DEATH) _actOnDeathPacket(context, &sendermac, rxbuflen);

				// Bye Packet
				else if(context->rxbuf[0] == ADHOC_MATCHING_PACKET_BYE) _actOnByePacket(context, &sendermac, delayed_removal);

				// Ignore Incoming Trash Data

				// Allow member access again
				sceKernelUnlockLwMutex(&members_lock, 1);
			}
			batch_process_packet_limit--;
		} while(recvresult == 0 && rxbuflen > 0 && batch_process_packet_limit > 0);

		// every second
		if (sceKernelGetSystemTimeWide() - last_adhocctl_check > 1000000)
		{
			last_adhocctl_check = sceKernelGetSystemTimeWide();
			// Timeout peers that are missing on adhocctl, in case they left without bye somehow
			timeout_missing_peers_on_adhocctl(context);
		}

		// Handle Peer Timeouts
		_handleTimeout(context);

		// We don't want incoming match making packets to pile up... do not delay too long here
		sceKernelDelayThread(9000);
	}
	
	// Send Bye Messages
	_sendByePacket(context);
	
	// Free Peer List Buffer
	_clearPeerList(context);
	
	// Delete Pointer Reference (and notify caller about finished cleanup)
	context->input_thid = 0;
	
	// Terminate Thread
	sceKernelExitDeleteThread(0);
	
	// Return Zero to shut up Compiler (never reached anyway)
	return 0;
}

/**
 * Handle Ping Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 */
void _actOnPingPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
	
	// Found Peer
	if(peer != NULL)
	{
		// Update Receive Timer
		peer->lastping = sceKernelGetSystemTimeWide();
		peer->last_seen_on_adhocctl = sceKernelGetSystemTimeWide();
	}
}

/**
 * Handle Hello Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnHelloPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length)
{
	// Interested in Hello Data
	if((context->mode == ADHOC_MATCHING_MODE_CHILD && _findParent(context) == NULL) || (context->mode == ADHOC_MATCHING_MODE_P2P && _findP2P(context) == NULL))
	{
		// Complete Packet Header available
		if(length >= 5)
		{
			// Extract Optional Data Length
			int optlen = 0; memcpy(&optlen, context->rxbuf + 1, sizeof(optlen));
			
			// Complete Valid Packet available
			if(optlen >= 0 && length >= (5 + optlen))
			{
				// Set Default Null Data
				void * opt = NULL;
				
				// Extract Optional Data Pointer
				if(optlen > 0) opt = context->rxbuf + 5;
				
				// Find Peer
				SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
				
				// Peer not found
				if(peer == NULL)
				{
					// Allocate Memory
					peer = (SceNetAdhocMatchingMemberInternal *)_malloc(sizeof(SceNetAdhocMatchingMemberInternal));
					
					// Allocated Memory
					if(peer != NULL)
					{
						// Clear Memory
						memset(peer, 0, sizeof(SceNetAdhocMatchingMemberInternal));
						
						// Copy Sender MAC
						peer->mac = *sendermac;
						
						// Set Peer State
						peer->state = ADHOC_MATCHING_PEER_OFFER;
						
						// Initialize Ping Timer
						peer->lastping = sceKernelGetSystemTimeWide();
						peer->last_seen_on_adhocctl = peer->lastping;

						// Link Peer into List
						peer->next = context->peerlist;
						context->peerlist = peer;
					}
				}
				
				// Peer available now
				if(peer != NULL)
				{
					// Spawn Hello Event
					_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_HELLO, sendermac, optlen, opt);
				}
			}
		}
	}
}

/**
 * Handle Join Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnJoinPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length)
{
	// Not a child mode context
	if(context->mode != ADHOC_MATCHING_MODE_CHILD)
	{
		// We still got a unoccupied slot in our room (Parent / P2P)
		if((context->mode == ADHOC_MATCHING_MODE_PARENT && _countChildren(context) < (context->maxpeers - 1)) || (context->mode == ADHOC_MATCHING_MODE_P2P && _findP2P(context) == NULL))
		{
			// Complete Packet Header available
			if(length >= 5)
			{
				// Extract Optional Data Length
				int optlen = 0; memcpy(&optlen, context->rxbuf + 1, sizeof(optlen));
				
				// Complete Valid Packet available
				if(optlen >= 0 && length >= (5 + optlen))
				{
					// Set Default Null Data
					void * opt = NULL;
					
					// Extract Optional Data Pointer
					if(optlen > 0) opt = context->rxbuf + 5;
					
					// Find Peer
					SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
					
					// If we got the peer in the table already and are a parent, there is nothing left to be done.
					// This is because the only way a parent can know of a child is via a join request...
					// If we thus know of a possible child, then we already had a previous join request thus no need for double tapping.
					if(peer != NULL && context->mode == ADHOC_MATCHING_MODE_PARENT) return;
					
					// New Peer
					if(peer == NULL)
					{
						// Allocate Memory
						peer = (SceNetAdhocMatchingMemberInternal *)_malloc(sizeof(SceNetAdhocMatchingMemberInternal));
						
						// Allocated Memory
						if(peer != NULL)
						{
							// Clear Memory
							memset(peer, 0, sizeof(SceNetAdhocMatchingMemberInternal));
							
							// Copy Sender MAC
							peer->mac = *sendermac;
							
							// Set Peer State
							peer->state = ADHOC_MATCHING_PEER_INCOMING_REQUEST;
							
							// Initialize Ping Timer
							peer->lastping = sceKernelGetSystemTimeWide();
							peer->last_seen_on_adhocctl = peer->lastping;
							
							// Link Peer into List
							peer->next = context->peerlist;
							context->peerlist = peer;
							
							// Spawn Request Event
							_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_REQUEST, sendermac, optlen, opt);
							
							// Return Success
							return;
						}
					}
					
					// Existing Peer (this case is only reachable for P2P mode)
					else
					{
						// Set Peer State
						peer->state = ADHOC_MATCHING_PEER_INCOMING_REQUEST;
						
						// Spawn Request Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_REQUEST, sendermac, optlen, opt);

						// Update timer
						peer->lastping = sceKernelGetSystemTimeWide();
						peer->last_seen_on_adhocctl = peer->lastping;

						// Return Success
						return;
					}
				}
			}
		}
		
		// Auto-Reject Player
		_sendCancelPacket(context, sendermac, 0, NULL);
	}
}

/**
 * Handle Accept Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnAcceptPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length)
{
	// Not a parent context
	if(context->mode != ADHOC_MATCHING_MODE_PARENT)
	{
		// Don't have a master yet
		if((context->mode == ADHOC_MATCHING_MODE_CHILD && _findParent(context) == NULL) || (context->mode == ADHOC_MATCHING_MODE_P2P && _findP2P(context) == NULL))
		{
			// Complete Packet Header available
			if(length >= 9)
			{
				// Extract Optional Data Length
				int optlen = 0; memcpy(&optlen, context->rxbuf + 1, sizeof(optlen));
				
				// Extract Sibling Count
				int siblingcount = 0; memcpy(&siblingcount, context->rxbuf + 5, sizeof(siblingcount));
				
				// Complete Valid Packet available
				if(optlen >= 0 && length >= (9 + optlen + sizeof(SceNetEtherAddr) * siblingcount))
				{
					// Set Default Null Data
					void * opt = NULL;
					
					// Extract Optional Data Pointer
					if(optlen > 0) opt = context->rxbuf + 9;
					
					// Sibling MAC Array Null Data
					SceNetEtherAddr * siblings = NULL;
					
					// Extract Optional Sibling MAC Array
					if(siblingcount > 0) siblings = (SceNetEtherAddr *)(context->rxbuf + 9 + optlen);
					
					// Find Outgoing Request
					SceNetAdhocMatchingMemberInternal * request = _findOutgoingRequest(context);
					
					// We are waiting for a answer to our request...
					if(request != NULL)
					{
						// Find Peer
						SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
						
						// It's the answer we wanted!
						if(request == peer)
						{
							// Change Peer State
							peer->state = (context->mode == ADHOC_MATCHING_MODE_CHILD) ? (ADHOC_MATCHING_PEER_PARENT) : (ADHOC_MATCHING_PEER_P2P);
							
							// Remove Unneeded Peer Information
							_postAcceptCleanPeerList(context);
							
							// Add Sibling Peers
							if(context->mode == ADHOC_MATCHING_MODE_CHILD)
							{
								_postAcceptAddSiblings(context, siblingcount, siblings);

								// PPSSPP adds self here
								SceNetEtherAddr local_mac = {0};
								sceNetGetLocalEtherAddr(&local_mac);
								SceNetAdhocMatchingMemberInternal *self = _findPeer(context, &local_mac);
								if (self == NULL)
								{
									SceNetAdhocMatchingMemberInternal *self = (SceNetAdhocMatchingMemberInternal *)_malloc(sizeof(SceNetAdhocMatchingMemberInternal));
									if (self != NULL)
									{
										memset(self, 0, sizeof(SceNetAdhocMatchingMemberInternal));
										// self->state = 0; // 0 state means self
										self->mac = local_mac;
										self->lastping = sceKernelGetSystemTimeWide();
										self->last_seen_on_adhocctl = self->lastping;
										self->next = context->peerlist;
										context->peerlist = self;
									}
								}
							}
							
							// IMPORTANT! The Event Order here is ok!
							// Internally the Event Stack appends to the front, so the order will be switched around.
							
							// Spawn Established Event
							_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_ESTABLISHED, sendermac, 0, NULL);
							
							// Spawn Accept Event
							_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_ACCEPT, sendermac, optlen, opt);
						}
					}
				}
			}
		}
	}
}

/**
 * Handle Cancel Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnCancelPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length, uint64_t delayed_removal)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
	
	// Get Parent
	SceNetAdhocMatchingMemberInternal * parent = _findParent(context);
	
	// Get Outgoing Join Request
	SceNetAdhocMatchingMemberInternal * request = _findOutgoingRequest(context);
	
	// Get P2P Partner
	SceNetAdhocMatchingMemberInternal * p2p = _findP2P(context);

	// Interest Condition fulfilled
	if(peer != NULL)
	{
		// Complete Packet Header available
		if(length >= 5)
		{
			// Extract Optional Data Length
			int optlen = 0; memcpy(&optlen, context->rxbuf + 1, sizeof(optlen));
			
			// Complete Valid Packet available
			if(optlen >= 0 && length >= (5 + optlen))
			{
				// Set Default Null Data
				void * opt = NULL;
				
				// Extract Optional Data Pointer
				if(optlen > 0) opt = context->rxbuf + 5;
				
				// Child Mode
				if(context->mode == ADHOC_MATCHING_MODE_CHILD)
				{
					// Join Request denied
					if(request == peer)
					{
						// Spawn Deny Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_DENY, sendermac, optlen, opt);
						
						// Delete Peer from List
						//_deletePeer(context, peer);
						peer->lastping = delayed_removal;
					}
					
					// Kicked from Room
					else if(parent == peer)
					{
						// Iterate Peers
						SceNetAdhocMatchingMemberInternal * item = context->peerlist; for(; item != NULL; item = item->next)
						{
							// Established Peer
							if(item->state == ADHOC_MATCHING_PEER_CHILD || item->state == ADHOC_MATCHING_PEER_PARENT)
							{
								// Spawn Leave / Kick Event
								_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_LEAVE, &item->mac, optlen, opt);
							}
						}
						
						// Delete Peer from List
						//_clearPeerList(context);

						SceNetAdhocMatchingMemberInternal *peer = context->peerlist;
						SceNetAdhocMatchingMemberInternal *last_peer = NULL;
						SceNetEtherAddr local_mac = {0};
						sceNetGetLocalEtherAddr(&local_mac);
						while(peer != NULL)
						{
							// Remove self
							if (_isMacMatch(&peer->mac, &local_mac))
							{
								SceNetAdhocMatchingMemberInternal *self = peer;
								if (peer == context->peerlist)
								{
									context->peerlist = peer->next;
								}
								else
								{
									last_peer->next = peer->next;
								}
								peer = peer->next;
								_free(self);
							}
							// Timeout others
							else
							{
								peer->lastping = delayed_removal;
								last_peer = peer;
								peer = peer->next;
							}
						}
					}
				}
				
				// Parent Mode
				else if(context->mode == ADHOC_MATCHING_MODE_PARENT)
				{
					// Cancel Join Request
					if(peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST)
					{
						// Spawn Request Cancel Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_CANCEL, sendermac, optlen, opt);
						
						// Delete Peer from List
						//_deletePeer(context, peer);
						peer->lastping = delayed_removal;
					}
					
					// Leave Room
					else if(peer->state == ADHOC_MATCHING_PEER_CHILD)
					{
						// Spawn Leave / Kick Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_LEAVE, sendermac, optlen, opt);
						
						// Delete Peer from List
						//_deletePeer(context, peer);
						peer->lastping = delayed_removal;
					}
				}
				
				// P2P Mode
				else
				{
					// Join Request denied
					if(request == peer)
					{
						// Spawn Deny Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_DENY, sendermac, optlen, opt);
						
						// Delete Peer from List
						//_deletePeer(context, peer);
						// PPSSPP timesout the peer here
						peer->lastping = delayed_removal;
					}
					
					// Kicked from Room
					else if(p2p == peer)
					{
						// Spawn Leave / Kick Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_LEAVE, sendermac, optlen, opt);
						
						// Delete Peer from List
						//_deletePeer(context, peer);
						peer->lastping = delayed_removal;
					}
					
					// Cancel Join Request
					else if(peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST)
					{
						// Spawn Request Cancel Event
						_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_CANCEL, sendermac, optlen, opt);
						
						// Delete Peer from List
						//_deletePeer(context, peer);
						peer->lastping = delayed_removal;
					}
				}
			}
		}
	}
}

/**
 * Handle Bulk Data Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnBulkDataPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
	
	// Established Peer
	if(peer != NULL && (
	(context->mode == ADHOC_MATCHING_MODE_PARENT && peer->state == ADHOC_MATCHING_PEER_CHILD) || 
	(context->mode == ADHOC_MATCHING_MODE_CHILD && (peer->state == ADHOC_MATCHING_PEER_CHILD || peer->state == ADHOC_MATCHING_PEER_PARENT)) || 
	(context->mode == ADHOC_MATCHING_MODE_P2P && peer->state == ADHOC_MATCHING_PEER_P2P)))
	{
		// Complete Packet Header available
		if(length > 5)
		{
			// Extract Data Length
			int datalen = 0; memcpy(&datalen, context->rxbuf + 1, sizeof(datalen));
			
			// Complete Valid Packet available
			if(datalen > 0 && length >= (5 + datalen))
			{
				// Extract Data
				void * data = context->rxbuf + 5;
				
				// Spawn Data Event
				_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_DATA, sendermac, datalen, data);
			}
		}
	}
}

/**
 * Handle Birth Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnBirthPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);

	// Extract Child MAC
	SceNetEtherAddr mac;
	memcpy(&mac, context->rxbuf + 1, sizeof(SceNetEtherAddr));

	// Valid Circumstances
	if(peer != NULL && context->mode == ADHOC_MATCHING_MODE_CHILD && peer == _findParent(context))
	{
		// Complete Packet available
		if(length >= (1 + sizeof(SceNetEtherAddr)))
		{
			// Allocate Memory (If this fails... we are fucked.)
			SceNetAdhocMatchingMemberInternal * sibling = (SceNetAdhocMatchingMemberInternal *)_malloc(sizeof(SceNetAdhocMatchingMemberInternal));
			
			// Allocated Memory
			if(sibling != NULL)
			{
				// Clear Memory
				memset(sibling, 0, sizeof(SceNetAdhocMatchingMemberInternal));
				
				// Save MAC Address
				sibling->mac = mac;
				
				// Set Peer State
				sibling->state = ADHOC_MATCHING_PEER_CHILD;
				
				// Initialize Ping Timer
				sibling->lastping = sceKernelGetSystemTimeWide();
				sibling->last_seen_on_adhocctl = sibling->lastping;
				
				// Link Peer
				sibling->next = context->peerlist;
				context->peerlist = sibling;

				printk("%s: created peer from the birth of %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, (uint32_t)mac.data[0], (uint32_t)mac.data[1], (uint32_t)mac.data[2], (uint32_t)mac.data[3], (uint32_t)mac.data[4], (uint32_t)mac.data[5]);

				// Spawn Established Event
				// no established on birth on PPSSPP
				//_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_ESTABLISHED, &sibling->mac, 0, NULL);
			}
		}
	}else{
		printk("%s: rejected birth packet from %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, (uint32_t)sendermac->data[0], (uint32_t)sendermac->data[1], (uint32_t)sendermac->data[2], (uint32_t)sendermac->data[3], (uint32_t)sendermac->data[4], (uint32_t)sendermac->data[5]);
		printk("%s: peer %p\n", __func__, peer);
		printk("%s: new born %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, (uint32_t)mac.data[0], (uint32_t)mac.data[1], (uint32_t)mac.data[2], (uint32_t)mac.data[3], (uint32_t)mac.data[4], (uint32_t)mac.data[5]);
	}
}

/**
 * Handle Death Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 * @param length Packet Length
 */
void _actOnDeathPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint32_t length)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);
	
	// Valid Circumstances
	if(peer != NULL && context->mode == ADHOC_MATCHING_MODE_CHILD && peer == _findParent(context))
	{
		// Complete Packet available
		if(length >= (1 + sizeof(SceNetEtherAddr)))
		{
			// Extract Child MAC
			SceNetEtherAddr mac;
			memcpy(&mac, context->rxbuf + 1, sizeof(SceNetEtherAddr));
			
			// Find Peer
			SceNetAdhocMatchingMemberInternal * deadkid = _findPeer(context, &mac);
			
			// Valid Sibling
			if(deadkid != NULL && deadkid->state == ADHOC_MATCHING_PEER_CHILD)
			{
				// Spawn Leave Event
				_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_LEAVE, &mac, 0, NULL);

				// Delete Peer
				_deletePeer(context, deadkid);
			}
		}
	}
}

/**
 * Handle Bye Packet
 * @param context Matching Context Pointer
 * @param sendermac Packet Sender MAC
 */
void _actOnByePacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * sendermac, uint64_t delayed_removal)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, sendermac);

	SceNetEtherAddr local_mac = {0};
	sceNetGetLocalEtherAddr(&local_mac);

	if (_isMacMatch(sendermac, &local_mac))
	{
		printk("%s: we are sending bye to ourself, please debug this\n", __func__);
		return;
	}
	else
	{
		printk("%s: bye from %x:%x:%x:%x:%x:%x\n", __func__, sendermac->data[0], sendermac->data[1], sendermac->data[2], sendermac->data[3], sendermac->data[4], sendermac->data[5]);
	}

	// We know this guy
	if(peer != NULL)
	{
		// P2P or Child Bye
		if((context->mode == ADHOC_MATCHING_MODE_PARENT && peer->state == ADHOC_MATCHING_PEER_CHILD) || 
		(context->mode == ADHOC_MATCHING_MODE_CHILD && peer->state == ADHOC_MATCHING_PEER_CHILD) ||
		/*(context->mode == ADHOC_MATCHING_MODE_P2P && peer->state == ADHOC_MATCHING_PEER_P2P))*/
		// PPSSPP logic
		(context->mode == ADHOC_MATCHING_MODE_P2P &&
		(peer->state == ADHOC_MATCHING_PEER_P2P || peer->state == ADHOC_MATCHING_PEER_OFFER || peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST || peer->state == ADHOC_MATCHING_PEER_OUTGOING_REQUEST || peer->state == ADHOC_MATCHING_PEER_CANCEL_IN_PROGRESS)))
		{
			// Spawn Leave / Kick Event
			_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_BYE, sendermac, 0, NULL);
			
			// Delete Peer
			//_deletePeer(context, peer);
			peer->lastping = delayed_removal;
		}
		
		// Parent Bye
		else if(context->mode == ADHOC_MATCHING_MODE_CHILD && peer->state == ADHOC_MATCHING_PEER_PARENT)
		{
			#if 0
			// Iterate Peers
			SceNetAdhocMatchingMemberInternal * item = context->peerlist; for(; item != NULL; item = item->next)
			{
				// Established Peer
				if(item->state == ADHOC_MATCHING_PEER_CHILD || item->state == ADHOC_MATCHING_PEER_PARENT)
				{
					// Spawn Leave / Kick Event
					_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_BYE, &item->mac, 0, NULL);
				}
			}
			#endif


			// Until we have very similar logic with PPSSPP, we just eat this

			// PPSSPP does one event only
			_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_BYE, sendermac, 0, NULL);
			
			// Delete Peer from List
			//_clearPeerList(context);

			SceNetAdhocMatchingMemberInternal *peer = context->peerlist;
			SceNetAdhocMatchingMemberInternal *last_peer = NULL;
			while(peer != NULL)
			{
				// Remove self
				if (_isMacMatch(&peer->mac, &local_mac))
				{
					SceNetAdhocMatchingMemberInternal *self = peer;
					if (peer == context->peerlist)
					{
						context->peerlist = peer->next;
					}
					else
					{
						last_peer->next = peer->next;
					}
					peer = peer->next;
					_free(self);
				}
				// Timeout others
				else
				{
					peer->lastping = delayed_removal;
					last_peer = peer;
					peer = peer->next;
				}
			}
		}
	}
}

/**
 * Remove unneeded Peer Data after being accepted to a match
 * @param context Matching Context Pointer
 */
void _postAcceptCleanPeerList(SceNetAdhocMatchingContext * context)
{
	// Iterate Peer List
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; while(peer != NULL)
	{
		// Save next Peer just in case we have to delete this one
		SceNetAdhocMatchingMemberInternal * next = peer->next;
		
		// Unneeded Peer
		//if(peer->state != ADHOC_MATCHING_PEER_CHILD && peer->state != ADHOC_MATCHING_PEER_P2P && peer->state != ADHOC_MATCHING_PEER_PARENT)
		// PPSSPP does not remove self here
		if(peer->state != ADHOC_MATCHING_PEER_CHILD && peer->state != ADHOC_MATCHING_PEER_P2P && peer->state != ADHOC_MATCHING_PEER_PARENT && peer->state != 0)
		{
			_deletePeer(context, peer);
		}
		
		// Move to Next Peer
		peer = next;
	}
}

/**
 * Add Sibling-Data that was sent with Accept-Datagram
 * @param context Matching Context Pointer
 * @param siblingcount Number of Siblings
 * @param siblings Sibling MAC Array
 */
void _postAcceptAddSiblings(SceNetAdhocMatchingContext * context, int siblingcount, SceNetEtherAddr * siblings)
{
	// Cast Sibling MAC Array to uint8_t
	// PSP CPU has a problem with non-4-byte aligned Pointer Access.
	// As the buffer of "siblings" isn't properly aligned I don't want to risk a crash.
	uint8_t * siblings_u8 = (uint8_t *)siblings;
	
	// Iterate Siblings
	//int i = 0; for(; i < siblingcount; i++)
	int i = siblingcount - 1; for(; i >= 0; i--) // reverse order like on PPSSPP
	{
		SceNetEtherAddr sibling_addr = {0};
		memcpy(&sibling_addr, siblings_u8 + sizeof(SceNetEtherAddr) * i, sizeof(SceNetEtherAddr));

		SceNetAdhocMatchingMemberInternal *sibling = _findPeer(context, &sibling_addr);
		if (sibling != NULL)
		{
			sibling->state = ADHOC_MATCHING_PEER_CHILD;
			sibling->sending = 0;
			sibling->lastping = sceKernelGetSystemTimeWide();
			sibling->last_seen_on_adhocctl = sibling->lastping;
			continue;
		}

		// Allocate Memory
		sibling = (SceNetAdhocMatchingMemberInternal *)_malloc(sizeof(SceNetAdhocMatchingMemberInternal));
		
		// Allocated Memory
		if(sibling != NULL)
		{
			// Clear Memory
			memset(sibling, 0, sizeof(SceNetAdhocMatchingMemberInternal));
			
			// Save MAC Address
			memcpy(&sibling->mac, siblings_u8 + sizeof(SceNetEtherAddr) * i, sizeof(SceNetEtherAddr));
			
			// Set Peer State
			sibling->state = ADHOC_MATCHING_PEER_CHILD;
			
			// Initialize Ping Timer
			sibling->lastping = sceKernelGetSystemTimeWide();
			sibling->last_seen_on_adhocctl = sibling->lastping;

			// Link Peer
			sibling->next = context->peerlist;
			context->peerlist = sibling;
			
			// Spawn Established Event
			// PPSSPP doesn't do that..
			//_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_ESTABLISHED, &sibling->mac, 0, NULL);
		}
	}
}

/**
 * Broadcast Ping Message to other Matching Users
 * @param context Matching Context Pointer
 */
void _broadcastPingMessage(SceNetAdhocMatchingContext * context)
{
	printk("%s: broadcasting ping\n", __func__);

	// Ping Opcode
	uint8_t ping = ADHOC_MATCHING_PACKET_PING;
	
	// Send Broadcast
	sceNetAdhocPdpSend(context->socket, (SceNetEtherAddr *)_broadcast, context->port, &ping, sizeof(ping), 0, ADHOC_F_NONBLOCK);
}

/**
 * Broadcast Hello Message to other Matching Users
 * @param context Matching Context Pointer
 */
void _broadcastHelloMessage(SceNetAdhocMatchingContext * context)
{
	// Allocate Hello Message Buffer
	uint8_t * hello = (uint8_t *)_malloc(5 + context->hellolen);
	
	// Allocated Hello Message Buffer
	if(hello != NULL)
	{
		printk("%s: broadcasting hello\n", __func__);

		// Hello Opcode
		hello[0] = ADHOC_MATCHING_PACKET_HELLO;
		
		// Hello Data Length (have to memcpy this to avoid cpu alignment crash)
		memcpy(hello + 1, &context->hellolen, sizeof(context->hellolen));
		
		// Copy Hello Data
		if(context->hellolen > 0) memcpy(hello + 5, context->hello, context->hellolen);
		
		// Send Broadcast
		sceNetAdhocPdpSend(context->socket, (SceNetEtherAddr *)_broadcast, context->port, hello, 5 + context->hellolen, 0, ADHOC_F_NONBLOCK);

		// Free Memory
		_free(hello);
	}
}

/**
 * Handle Timeouts in Matching Context
 * @param context Matchi]ng Context Pointer
 */
void _handleTimeout(SceNetAdhocMatchingContext * context)
{
	if (context->keepalive_int == 0){
		// on PPSSPP, timeout does not happen when keepalive_int is 0
		return;
	}

	// Iterate Peer List
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; while(peer != NULL)
	{
		// Get Next Pointer (to avoid crash on memory freeing)
		SceNetAdhocMatchingMemberInternal * next = peer->next;

		uint64_t now = sceKernelGetSystemTimeWide();

		// Timeout!
		if((now > peer->lastping && now - peer->lastping >= context->timeout) || peer->lastping == 0)
		{
			// Spawn Timeout Event
			// sync logic with PPSSPP
			if(
			/*(context->mode == ADHOC_MATCHING_MODE_CHILD && (peer->state == ADHOC_MATCHING_PEER_CHILD || peer->state == ADHOC_MATCHING_PEER_PARENT)) || */
			(context->mode == ADHOC_MATCHING_MODE_CHILD) && (peer->state == ADHOC_MATCHING_MODE_PARENT)  ||
			(context->mode == ADHOC_MATCHING_MODE_PARENT && peer->state == ADHOC_MATCHING_PEER_CHILD) || 
			/*(context->mode == ADHOC_MATCHING_MODE_P2P && peer->state == ADHOC_MATCHING_PEER_P2P))*/
			(context->mode == ADHOC_MATCHING_MODE_P2P && 
			(peer->state == ADHOC_MATCHING_PEER_P2P || peer->state == ADHOC_MATCHING_PEER_OFFER || peer->state == ADHOC_MATCHING_PEER_INCOMING_REQUEST || peer->state == ADHOC_MATCHING_PEER_OUTGOING_REQUEST || peer->state == ADHOC_MATCHING_PEER_CANCEL_IN_PROGRESS)))
			_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_TIMEOUT, &peer->mac, 0, NULL);

			SceNetEtherAddr local_mac = {0};
			sceNetGetLocalEtherAddr(&local_mac);
			// Don't timeout self if in list
			if(!_isMacMatch(&local_mac, &peer->mac))
			{
				printk("%s: timing out %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, (uint32_t)peer->mac.data[0], (uint32_t)peer->mac.data[1], (uint32_t)peer->mac.data[2], (uint32_t)peer->mac.data[3], (uint32_t)peer->mac.data[4], (uint32_t)peer->mac.data[5]);
				printk("%s: now %u lastping %u timeout %u\n", __func__, (uint32_t)now, (uint32_t)peer->lastping, (uint32_t)context->timeout);

				// PPSSPP send messages here
				if (context->mode == ADHOC_MATCHING_MODE_PARENT)
				{
					_sendDeathPacket(context, &peer->mac);
				}
				else
				{
					_sendCancelPacket(context, &peer->mac, 0, NULL);
				}
			}

			// Need to keep peer to send the final messages
			// Delete Peer from List
			//_deletePeer(context, peer);
		}
		
		// Move Pointer
		peer = next;
	}
}

/**
 * Send Accept Packet to Player
 * @param context Matching Context Pointer
 * @param mac Target Player MAC
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendAcceptPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int optlen, void * opt)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, mac);
	
	// Found Peer
	if(peer != NULL && (peer->state == ADHOC_MATCHING_PEER_CHILD || peer->state == ADHOC_MATCHING_PEER_P2P))
	{
		// Required Sibling Buffer
		uint32_t siblingbuflen = 0;
		
		// Parent Mode
		if(context->mode == ADHOC_MATCHING_MODE_PARENT) siblingbuflen = sizeof(SceNetEtherAddr) * (_countConnectedPeers(context) - 2);
		
		// Sibling Count
		int siblingcount = siblingbuflen / sizeof(SceNetEtherAddr);
		
		// Allocate Accept Message Buffer
		uint8_t * accept = (uint8_t *)_malloc(9 + optlen + siblingbuflen);
	
		// Allocated Accept Message Buffer
		if(accept != NULL)
		{
			// Accept Opcode
			accept[0] = ADHOC_MATCHING_PACKET_ACCEPT;
			
			// Optional Data Length
			memcpy(accept + 1, &optlen, sizeof(optlen));
			
			// Sibling Count
			memcpy(accept + 5, &siblingcount, sizeof(siblingcount));
			
			// Copy Optional Data
			if(optlen > 0) memcpy(accept + 9, opt, optlen);
			
			// Parent Mode Extra Data required
			if(context->mode == ADHOC_MATCHING_MODE_PARENT && siblingcount > 0)
			{
				// Create MAC Array Pointer
				uint8_t * siblingmacs = (uint8_t *)(accept + 9 + optlen);
				
				// MAC Writing Pointer
				int i = 0;
				
				// Iterate Peer List
				SceNetAdhocMatchingMemberInternal * item = context->peerlist; for(; item != NULL; item = item->next)
				{
					// Ignore Target
					if(item == peer) continue;
					
					// Copy Child MAC
					if(item->state == ADHOC_MATCHING_PEER_CHILD)
					{
						// Clone MAC the stupid memcpy way to shut up PSP CPU
						memcpy(siblingmacs + sizeof(SceNetEtherAddr) * i++, &item->mac, sizeof(SceNetEtherAddr));
					}
				}
			}
			
			// Send Data
			sceNetAdhocPdpSend(context->socket, mac, context->port, accept, 9 + optlen + siblingbuflen, 0, ADHOC_F_NONBLOCK);
			
			// Free Memory
			_free(accept);
			
			// Spawn Local Established Event
			_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_ESTABLISHED, mac, 0, NULL);
		}
	}
}

/**
 * Send Join Packet to Player
 * @param context Matching Context Pointer
 * @param mac Target Player MAC
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendJoinPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int optlen, void * opt)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, mac);
	
	// Valid Peer
	if(peer != NULL && peer->state == ADHOC_MATCHING_PEER_OUTGOING_REQUEST)
	{
		// Allocate Join Message Buffer
		uint8_t * join = (uint8_t *)_malloc(5 + optlen);
		
		// Allocated Join Message Buffer
		if(join != NULL)
		{
			// Join Opcode
			join[0] = ADHOC_MATCHING_PACKET_JOIN;
			
			// Optional Data Length
			memcpy(join + 1, &optlen, sizeof(optlen));
			
			// Copy Optional Data
			if(optlen > 0) memcpy(join + 5, opt, optlen);
			
			// Send Data
			sceNetAdhocPdpSend(context->socket, mac, context->port, join, 5 + optlen, 0, ADHOC_F_NONBLOCK);

			// Free Memory
			_free(join);
		}
	}
}

/**
 * Send Cancel Packet to Player
 * @param context Matching Context Pointer
 * @param mac Target Player MAC
 * @param optlen Optional Data Length
 * @param opt Optional Data
 */
void _sendCancelPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int optlen, void * opt)
{
	// Allocate Cancel Message Buffer
	uint8_t * cancel = (uint8_t *)_malloc(5 + optlen);
	
	// Allocated Cancel Message Buffer
	if(cancel != NULL)
	{
		// Cancel Opcode
		cancel[0] = ADHOC_MATCHING_PACKET_CANCEL;
		
		// Optional Data Length
		memcpy(cancel + 1, &optlen, sizeof(optlen));
		
		// Copy Optional Data
		if(optlen > 0) memcpy(cancel + 5, opt, optlen);
		
		// Send Data
		sceNetAdhocPdpSend(context->socket, mac, context->port, cancel, 5 + optlen, 0, ADHOC_F_NONBLOCK);

		// Free Memory
		_free(cancel);
	}
	
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, mac);
	
	// Found Peer
	if(peer != NULL)
	{
		// Child Mode Fallback - Delete All
		if(context->mode == ADHOC_MATCHING_MODE_CHILD)
		{
			// Delete Peer List
			_clearPeerList(context);
		}
		
		// Delete Peer
		else _deletePeer(context, peer);
	}
}

/**
 * Send Bulk Data Packet to Player
 * @param context Matching Context Pointer
 * @param mac Target Player MAC
 * @param datalen Data Length
 * @param data Data
 */
void _sendBulkDataPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac, int datalen, void * data)
{
	// Find Peer
	SceNetAdhocMatchingMemberInternal * peer = _findPeer(context, mac);
	
	// Valid Peer (rest is already checked in send.c)
	if(peer != NULL)
	{
		// Allocate Send Message Buffer
		uint8_t * send = (uint8_t *)_malloc(5 + datalen);
		
		// Allocated Send Message Buffer
		if(send != NULL)
		{
			// Send Opcode
			send[0] = ADHOC_MATCHING_PACKET_BULK;
			
			// Data Length
			memcpy(send + 1, &datalen, sizeof(datalen));
			
			// Copy Data
			memcpy(send + 5, data, datalen);
			
			// Send Data
			sceNetAdhocPdpSend(context->socket, mac, context->port, send, 5 + datalen, 0, ADHOC_F_NONBLOCK);

			// Free Memory
			_free(send);
			
			// Remove Busy Bit from Peer
			peer->sending = 0;
			
			// Spawn Data Event
			_spawnLocalEvent(context, ADHOC_MATCHING_EVENT_DATA_ACK, mac, 0, NULL);
		}
	}
}

/**
 * Tell Established Peers of new Child
 * @param context Matching Context Pointer
 * @param mac New Child's MAC
 */
void _sendBirthPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac)
{
	// Find Newborn Child
	SceNetAdhocMatchingMemberInternal * newborn = _findPeer(context, mac);
	
	// Found Newborn Child
	if(newborn != NULL)
	{
		// Packet Buffer
		uint8_t packet[7];
		
		// Set Opcode
		packet[0] = ADHOC_MATCHING_PACKET_BIRTH;
		
		// Set Newborn MAC
		memcpy(packet + 1, mac, sizeof(SceNetEtherAddr));
		
		// Iterate Peers
		SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
		{
			// Skip Newborn Child
			if(peer == newborn) continue;
			
			// Send only to children
			if(peer->state == ADHOC_MATCHING_PEER_CHILD)
			{
				printk("%s: sending birth of %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, (uint32_t)mac->data[0], (uint32_t)mac->data[1], (uint32_t)mac->data[2], (uint32_t)mac->data[3], (uint32_t)mac->data[4], (uint32_t)mac->data[5]);
				printk("%s: to %02x:%02x:%02x:%02x:%02x:%02x\n", __func__, (uint32_t)peer->mac.data[0], (uint32_t)peer->mac.data[1], (uint32_t)peer->mac.data[2], (uint32_t)peer->mac.data[3], (uint32_t)peer->mac.data[4], (uint32_t)peer->mac.data[5]);

				// Send Packet
				sceNetAdhocPdpSend(context->socket, &peer->mac, context->port, packet, sizeof(packet), 0, ADHOC_F_NONBLOCK);
			}
		}
	}
}

/**
 * Tell Established Peers of abandoned Child
 * @param context Matching Context Pointer
 * @param mac Dead Child's MAC
 */
void _sendDeathPacket(SceNetAdhocMatchingContext * context, SceNetEtherAddr * mac)
{
	// Find abandoned Child
	SceNetAdhocMatchingMemberInternal * deadkid = _findPeer(context, mac);
	
	// Found abandoned Child
	if(deadkid != NULL)
	{
		// Packet Buffer
		uint8_t packet[7];
		
		// Set abandoned Child MAC
		memcpy(packet + 1, mac, sizeof(SceNetEtherAddr));
		
		// Iterate Peers
		SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
		{
			// Skip dead Child
			//if(peer == deadkid) continue;

			// Send bye as PPSSPP does
			if(peer == deadkid)
			{
				packet[0] = ADHOC_MATCHING_PACKET_BYE;
				// Send Packet
				sceNetAdhocPdpSend(context->socket, &peer->mac, context->port, packet, sizeof(packet[0]), 0, ADHOC_F_NONBLOCK);
				continue;
			}

			// Send only to children
			if(peer->state == ADHOC_MATCHING_PEER_CHILD)
			{
				// Set Opcode
				packet[0] = ADHOC_MATCHING_PACKET_DEATH;

				// Send Packet
				sceNetAdhocPdpSend(context->socket, &peer->mac, context->port, packet, sizeof(packet), 0, ADHOC_F_NONBLOCK);
			}
		}

		// Delete peer
		_deletePeer(context, deadkid);
	}
}

/**
 * Tell Established Peers that we're shutting the Networking Layer down
 * @param context Matching Context Pointer
 */
void _sendByePacket(SceNetAdhocMatchingContext * context)
{
	// Iterate Peers
	SceNetAdhocMatchingMemberInternal * peer = context->peerlist; for(; peer != NULL; peer = peer->next)
	{
		// Peer of Interest
		if(peer->state == ADHOC_MATCHING_PEER_PARENT || peer->state == ADHOC_MATCHING_PEER_CHILD || peer->state == ADHOC_MATCHING_PEER_P2P)
		{
			// Bye Opcode
			uint8_t opcode = ADHOC_MATCHING_PACKET_BYE;
			
			// Send Bye Packet
			sceNetAdhocPdpSend(context->socket, &peer->mac, context->port, &opcode, sizeof(opcode), 0, ADHOC_F_NONBLOCK);
		}
	}
}

uint64_t get_last_matching_packet_send(){
	return last_packet_send;
}
