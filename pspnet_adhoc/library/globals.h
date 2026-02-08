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

#ifndef _ADHOC_GLOBALS_H_
#define _ADHOC_GLOBALS_H_

// Error Codes
#define ADHOC_INVALID_SOCKET_ID 0x80410701
#define ADHOC_INVALID_ADDR 0x80410702
#define ADHOC_INVALID_PORT 0x80410703
#define ADHOC_INVALID_DATALEN 0x80410705
#define ADHOC_NOT_ENOUGH_SPACE 0x80400706
#define ADHOC_SOCKET_DELETED 0x80410707
#define ADHOC_SOCKET_ALERTED 0x80410708
#define ADHOC_WOULD_BLOCK 0x80410709
#define ADHOC_PORT_IN_USE 0x8041070A
#define ADHOC_NOT_CONNECTED 0x8041070B
#define ADHOC_DISCONNECTED 0x8041070C
#define ADHOC_NOT_OPENED 0x8040070D
#define ADHOC_NOT_LISTENED 0x8040070E
#define ADHOC_SOCKET_ID_NOT_AVAIL 0x8041070F
#define ADHOC_PORT_NOT_AVAIL 0x80410710
#define ADHOC_INVALID_ARG 0x80410711
#define ADHOC_NOT_INITIALIZED 0x80410712
#define ADHOC_ALREADY_INITIALIZED 0x80410713
#define ADHOC_BUSY 0x80410714
#define ADHOC_TIMEOUT 0x80410715
#define ADHOC_EXCEPTION_EVENT 0x80410717
#define ADHOC_CONNECTION_REFUSED 0x80410718
#define ADHOC_THREAD_ABORTED 0x80410719
#define ADHOC_ALREADY_CREATED 0x8041071A
#define ADHOC_NOT_IN_GAMEMODE 0x8041071B
#define ADHOC_NOT_CREATED 0x8041071C
#define WLAN_INVALID_ARG 0x80410D13

// Event Flags
#define ADHOC_EV_SEND 1
#define ADHOC_EV_RECV 2
#define ADHOC_EV_ALERT 0x400
#define ADHOC_EV_DISCONNECT 0x0800
#define ADHOC_EV_INVALID 0x0100
#define ADHOC_EV_CONNECT 0x0004
#define ADHOC_EV_ACCEPT 0x0008

// PTP Connection States
#define PTP_STATE_CLOSED 0
#define PTP_STATE_LISTEN 1
#define PTP_STATE_ESTABLISHED 4
#define PTP_STATE_SYN_SENT 2

// Alert Flags
#define ADHOC_F_ALERTSEND 0x0010
#define ADHOC_F_ALERTRECV 0x0020
#define ADHOC_F_ALERTPOLL 0x0040
#define ADHOC_F_ALERTCONNECT 0x0080
#define ADHOC_F_ALERTACCEPT 0x0100
#define ADHOC_F_ALERTFLUSH 0x0200
#define ADHOC_F_ALERTALL (ADHOC_F_ALERTSEND | ADHOC_F_ALERTRECV | ADHOC_F_ALERTPOLL | ADHOC_F_ALERTCONNECT | ADHOC_F_ALERTACCEPT | ADHOC_F_ALERTFLUSH)

// postoffice port
#define POSTOFFICE_PORT 27313

// Sockets
extern AdhocSocket *_sockets[255];

// Gamemode Buffer
extern SceNetAdhocGameModeBufferStat * _gmb;

// Initialized Switch
extern int _init;

// Manage Infrastructure Modules Switch
extern int _manage_modules;

// Global One
extern int _one;

// Global Zero
extern int _zero;

// One gamemode state
extern GamemodeInternal _gamemode;
extern SceLwMutexWorkarea _gamemode_lock;
extern SceUID _gamemode_socket;
extern int _gamemode_socket_users;

#define ADHOC_GAMEMODE_PORT 31000
#define GAMEMODE_UPDATE_INTERVAL_USEC 12000
#define GAMEMODE_INIT_DELAY_USEC 100000
#define GAMEMODE_SYNC_TIMEOUT_USEC 250000

// Broadcast mac
extern const SceNetEtherAddr _broadcast_mac;

extern GamemodeInternal *_gamemode_replicas[256];
extern int _gamemode_stop_thread;
extern SceUID _gamemode_thread_id;
extern int _gamemode_replica_stop_thread;
extern SceUID _gamemode_replica_thread_id;
extern int _gamemode_num_peers;
extern int _gamemode_all_received;
extern SceUID _socket_mapper_mutex;
extern SceUID _server_resolve_mutex;
extern int _port_offset;
extern int _is_ppsspp;
extern int _postoffice;
extern int _vita_speedup;

#endif
