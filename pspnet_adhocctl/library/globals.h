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

#ifndef _ADHOCCTL_GLOBALS_H_
#define _ADHOCCTL_GLOBALS_H_

// Adhoc-Control Metaport
#define ADHOCCTL_METAPORT 27312
#define ADHOCCTL_RECV_TIMEOUT 100000
#define ADHOCCTL_PING_TIMEOUT 2000000

// Error Codes
#define ADHOC_NO_ENTRY 0x80410716
#define ADHOCCTL_INVALID_ARG 0x80410B04
#define ADHOCCTL_ID_NOT_FOUND 0x80410B06
#define ADHOCCTL_ALREADY_INITIALIZED 0x80410B07
#define ADHOCCTL_NOT_INITIALIZED 0x80410B08
#define ADHOCCTL_NOT_ENTER_GAMEMODE 0x80410B0C
#define ADHOCCTL_CHANNEL_NOT_AVAILABLE 0x80410B0D
#define ADHOCCTL_BUSY 0x80410B10
#define ADHOCCTL_HANDLER_MAX 0x80410B12
#define ADHOCCTL_STACKSIZE_TOO_SHORT 0x80410B13

// Library State
extern int _init;

// Thread Status
extern int _thread_status;

// UPNP Library Handle
extern int _upnp_uid;

// Game Product Code
extern SceNetAdhocctlAdhocId _product_code;

// Game Group
extern SceNetAdhocctlParameter _parameter;

// Peer List
extern SceNetAdhocctlPeerInfoEmu * _friends;

// Scan Network List
extern SceNetAdhocctlScanInfo * _networks;
extern SceNetAdhocctlScanInfo * _newnetworks;

// Event Handler
#define ADHOCCTL_MAX_HANDLER 8
extern SceNetAdhocctlHandler _event_handler[ADHOCCTL_MAX_HANDLER];
extern int _event_handler_gp[ADHOCCTL_MAX_HANDLER];
extern void * _event_args[ADHOCCTL_MAX_HANDLER];

// Access Point Setting
extern int _hotspot;

// Meta Socket
extern int _metasocket;

//#ifdef ENABLE_PEERLOCK
#if 0
// Peer Locker
extern int _peerlock;
#endif

#ifdef ENABLE_NETLOCK
// Network Locker
extern int _networklock;
#endif

// Netconf Status
extern int _netconf_status;
extern SceUtilityNetconfParam _netconf_param;
extern SceUtilityNetconfAdhocParam _netconf_adhoc_param;

// Bit-Values
extern int _one;
extern int _zero;

extern int _num_gamemode_peers;
extern SceNetEtherAddr _gamemode_peers[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
extern int _num_actual_gamemode_peers;
extern SceNetEtherAddr _actual_gamemode_peers[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
extern int _joining_gamemode;
extern int _in_gamemode;
extern SceNetEtherAddr _gamemode_host;
extern int _gamemode_host_arrived;
extern int _gamemode_self_arrived;
extern int _gamemode_notified;

extern SceLwMutexWorkarea networking_lock;
extern SceLwMutexWorkarea peer_lock;
extern SceLwMutexWorkarea group_list_lock;

extern uint64_t _disconnect_timestamp;
extern uint64_t _gamemode_join_timestamp;

#endif
