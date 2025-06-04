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

#ifndef _ADHOC_MATCHING_GLOBALS_H_
#define _ADHOC_MATCHING_GLOBALS_H_

// Error Codes
#define ADHOC_MATCHING_INVALID_MODE 0x80410801
#define ADHOC_MATCHING_INVALID_MAXNUM 0x80410803
#define ADHOC_MATCHING_RXBUF_TOO_SHORT 0x80410804
#define ADHOC_MATCHING_INVALID_OPTLEN 0x80410805
#define ADHOC_MATCHING_INVALID_ARG 0x80410806
#define ADHOC_MATCHING_INVALID_ID 0x80410807
#define ADHOC_MATCHING_ID_NOT_AVAIL 0x80410808
#define ADHOC_MATCHING_NO_SPACE 0x80410809
#define ADHOC_MATCHING_IS_RUNNING 0x8041080A
#define ADHOC_MATCHING_NOT_RUNNING 0x8041080B
#define ADHOC_MATCHING_UNKNOWN_TARGET 0x8041080C
#define ADHOC_MATCHING_TARGET_NOT_READY 0x8041080D
#define ADHOC_MATCHING_EXCEED_MAXNUM 0x8041080E
#define ADHOC_MATCHING_REQUEST_IN_PROGRESS 0x8041080F
#define ADHOC_MATCHING_ALREADY_ESTABLISHED 0x80410810
#define ADHOC_MATCHING_BUSY 0x80410811
#define ADHOC_MATCHING_ALREADY_INITIALIZED 0x80410812
#define ADHOC_MATCHING_NOT_INITIALIZED 0x80410813
#define ADHOC_MATCHING_PORT_IN_USE 0x80410814
#define ADHOC_MATCHING_STACKSIZE_TOO_SHORT 0x80410815
#define ADHOC_MATCHING_INVALID_DATALEN 0x80410816
#define ADHOC_MATCHING_NOT_ESTABLISHED 0x80410817
#define ADHOC_MATCHING_DATA_BUSY 0x80410818

// Matching Modes
#define ADHOC_MATCHING_MODE_PARENT 1
#define ADHOC_MATCHING_MODE_CHILD 2
#define ADHOC_MATCHING_MODE_P2P 3

// Matching Events
#define ADHOC_MATCHING_EVENT_HELLO 1
#define ADHOC_MATCHING_EVENT_REQUEST 2
#define ADHOC_MATCHING_EVENT_LEAVE 3
#define ADHOC_MATCHING_EVENT_DENY 4
#define ADHOC_MATCHING_EVENT_CANCEL 5
#define ADHOC_MATCHING_EVENT_ACCEPT 6
#define ADHOC_MATCHING_EVENT_ESTABLISHED 7
#define ADHOC_MATCHING_EVENT_TIMEOUT 8
#define ADHOC_MATCHING_EVENT_ERROR 9
#define ADHOC_MATCHING_EVENT_BYE 10
#define ADHOC_MATCHING_EVENT_DATA 11
#define ADHOC_MATCHING_EVENT_DATA_ACK 12
#define ADHOC_MATCHING_EVENT_DATA_TIMEOUT 13

// Peer Status
// Offer only seen in P2P and PARENT mode after hello
// Parent only seen in CHILD mode after connection accept
// Child only seen in PARENT and CHILD mode after connection accept
// P2P only seen in P2P mode after connection accept
// Requester only seen in P2P and PARENT mode after connection request
#define ADHOC_MATCHING_PEER_OFFER 1
#define ADHOC_MATCHING_PEER_PARENT 2
#define ADHOC_MATCHING_PEER_CHILD 3
#define ADHOC_MATCHING_PEER_P2P 4
#define ADHOC_MATCHING_PEER_INCOMING_REQUEST 5
#define ADHOC_MATCHING_PEER_OUTGOING_REQUEST 6
#define ADHOC_MATCHING_PEER_CANCEL_IN_PROGRESS 7

// Packet Opcodes
#define ADHOC_MATCHING_PACKET_PING 0
#define ADHOC_MATCHING_PACKET_HELLO 1
#define ADHOC_MATCHING_PACKET_JOIN 2
#define ADHOC_MATCHING_PACKET_ACCEPT 3
#define ADHOC_MATCHING_PACKET_CANCEL 4
#define ADHOC_MATCHING_PACKET_BULK 5
#define ADHOC_MATCHING_PACKET_BULK_ABORT 6
#define ADHOC_MATCHING_PACKET_BIRTH 7
#define ADHOC_MATCHING_PACKET_DEATH 8
#define ADHOC_MATCHING_PACKET_BYE 9

// Stack Targets
#define ADHOC_MATCHING_INPUT_STACK 1
#define ADHOC_MATCHING_EVENT_STACK 2

// Library State
extern int _init;

// Fake Pool Size
extern uint32_t _fake_poolsize;

// Context List
extern SceNetAdhocMatchingContext * _contexts;

#endif
