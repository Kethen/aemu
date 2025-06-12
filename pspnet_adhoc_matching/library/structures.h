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

#ifndef _ADHOC_MATCHING_STRUCTURES_H_
#define _ADHOC_MATCHING_STRUCTURES_H_

// Ethernet Address
#define ETHER_ADDR_LEN 6
typedef struct SceNetEtherAddr {
	uint8_t data[ETHER_ADDR_LEN];
} __attribute__((packed)) SceNetEtherAddr;

// Malloc Pool Information
typedef struct SceNetMallocStat {
	int poolsize;
	int maxsize;
	int freesize;
} __attribute__((packed)) SceNetMallocStat;

// Matching Peer Information
typedef struct SceNetAdhocMatchingMember {
	struct SceNetAdhocMatchingMember * next;
	SceNetEtherAddr addr;
	uint8_t padding[2];
} __attribute__((packed)) SceNetAdhocMatchingMember;

// Internal Matching Peer Information
typedef struct SceNetAdhocMatchingMemberInternal {
	// Next Peer
	struct SceNetAdhocMatchingMemberInternal * next;
	
	// MAC Address
	SceNetEtherAddr mac;
	
	// State Variable
	int state;
	
	// Send in Progress
	int sending;
	
	// Last Heartbeat
	uint64_t lastping;
} SceNetAdhocMatchingMemberInternal;

// Adhoc Matching Handler
typedef void(*SceNetAdhocMatchingHandler)(int id, int event, SceNetEtherAddr * peer, int optlen, void * opt);

// Thread Message Stack Item
typedef struct ThreadMessage
{
	// Next Thread Message
	struct ThreadMessage * next;
	
	// Stack Event Opcode
	uint32_t opcode;
	
	// Target MAC Address
	SceNetEtherAddr mac;
	
	// Optional Data Length
	int optlen;
} ThreadMessage;

// Established Peer

// Context Information
typedef struct SceNetAdhocMatchingContext {
	// Next Context
	struct SceNetAdhocMatchingContext * next;
	
	// Externally Visible ID
	int id;
	
	// Matching Mode (HOST, CLIENT, P2P)
	int mode;
	
	// Running Flag (1 = running, 0 = created)
	int running;
	
	// Maximum Number of Peers (for HOST, P2P)
	int maxpeers;
	
	// Local MAC Address
	SceNetEtherAddr mac;
	
	// Peer List for Connectees
	SceNetAdhocMatchingMemberInternal * peerlist;
	
	// Local PDP Port
	uint16_t port;
	
	// Local PDP Socket
	int socket;
	
	// Receive Buffer Length
	int rxbuflen;
	
	// Receive Buffer
	uint8_t * rxbuf;
	
	// Hello Broadcast Interval (Microseconds)
	uint32_t hello_int;
	
	// Keep-Alive Broadcast Interval (Microseconds)
	uint32_t keepalive_int;
	
	// Resend Interval (Microseconds)
	uint32_t resend_int;
	
	// Resend-Counter
	int resendcounter;
	
	// Keep-Alive Counter
	int keepalivecounter;
	
	// Event Handler
	SceNetAdhocMatchingHandler handler;
	
	// Hello Data Length
	uint32_t hellolen;
	
	// Hello Data
	void * hello;
	
	// Event Caller Thread
	int event_thid;
	
	// IO Handler Thread
	int input_thid;
	
	// Event Caller Thread Message Stack
	int event_stack_lock;
	ThreadMessage * event_stack;
	
	// IO Handler Thread Message Stack
	int input_stack_lock;
	ThreadMessage * input_stack;

	// gp value the event handler expects...
	int gp_value;
} SceNetAdhocMatchingContext;

#endif

