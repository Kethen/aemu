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

#ifndef _ADHOCCTL_STRUCTURES_H_
#define _ADHOCCTL_STRUCTURES_H_

// Network Types for Scan Procedure
#define ADHOCCTL_MODE_ADHOC 0
#define ADHOCCTL_MODE_GAMEMODE 1

// Event Types for Event Handler
#define ADHOCCTL_EVENT_ERROR 0
#define ADHOCCTL_EVENT_CONNECT 1
#define ADHOCCTL_EVENT_DISCONNECT 2
#define ADHOCCTL_EVENT_SCAN 3
#define ADHOCCTL_EVENT_GAMEMODE 4

#define ERROR_NET_ADHOC_TIMEOUT 0x80410715

// Internal Thread States
#define ADHOCCTL_STATE_DISCONNECTED 0
#define ADHOCCTL_STATE_CONNECTED 1
#define ADHOCCTL_STATE_SCANNING 2
#define ADHOCCTL_STATE_GAMEMODE 3

// Kernel Utility Netconf Adhoc Types
#define UTILITY_NETCONF_TYPE_CONNECT_ADHOC 2
#define UTILITY_NETCONF_TYPE_CREATE_ADHOC 4
#define UTILITY_NETCONF_TYPE_JOIN_ADHOC 5

// Kernel Utility States
#define UTILITY_NETCONF_STATUS_NONE 0
#define UTILITY_NETCONF_STATUS_INITIALIZE 1
#define UTILITY_NETCONF_STATUS_RUNNING 2
#define UTILITY_NETCONF_STATUS_FINISHED 3
#define UTILITY_NETCONF_STATUS_SHUTDOWN 4

// from PPSSPP
#define SCE_ERROR_UTILITY_INVALID_STATUS 0x80110001
#define SCE_ERROR_UTILITY_WRONG_TYPE 0x80110005


// Ethernet Address
#define ETHER_ADDR_LEN 6
typedef struct SceNetEtherAddr {
	uint8_t data[ETHER_ADDR_LEN];
} __attribute__((packed)) SceNetEtherAddr;

// Adhoc ID (Game Product Key)
#define ADHOCCTL_ADHOCID_LEN 9
typedef struct SceNetAdhocctlAdhocId {
	int type;
	uint8_t data[ADHOCCTL_ADHOCID_LEN];
	uint8_t padding[3];
} SceNetAdhocctlAdhocId;

// Adhoc Virtual Network Name
#define ADHOCCTL_GROUPNAME_LEN 8
typedef struct SceNetAdhocctlGroupName {
	uint8_t data[ADHOCCTL_GROUPNAME_LEN];
} __attribute__((packed)) SceNetAdhocctlGroupName;

// Virtual Network Host Information
typedef struct SceNetAdhocctlBSSId {
	SceNetEtherAddr mac_addr;
	uint8_t padding[2];
} __attribute__((packed)) SceNetAdhocctlBSSId;

// Virtual Network Information
typedef struct SceNetAdhocctlScanInfo {
	struct SceNetAdhocctlScanInfo * next;
	int channel;
	SceNetAdhocctlGroupName group_name;
	SceNetAdhocctlBSSId bssid;
	int mode;
} __attribute__((packed)) SceNetAdhocctlScanInfo;

// Player Nickname
#define ADHOCCTL_NICKNAME_LEN 128
typedef struct SceNetAdhocctlNickname {
	uint8_t data[ADHOCCTL_NICKNAME_LEN];
} __attribute__((packed)) SceNetAdhocctlNickname;

// Active Virtual Network Information
typedef struct SceNetAdhocctlParameter {
	int channel;
	SceNetAdhocctlGroupName group_name;
	SceNetAdhocctlBSSId bssid;
	SceNetAdhocctlNickname nickname;
} __attribute__((packed)) SceNetAdhocctlParameter;

// Peer Information
typedef struct SceNetAdhocctlPeerInfoEmu {
	struct SceNetAdhocctlPeerInfoEmu * next;
	SceNetAdhocctlNickname nickname;
	SceNetEtherAddr mac_addr;
	uint32_t ip_addr;
	uint8_t padding[2];
	uint64_t last_recv;
} __attribute__((packed)) SceNetAdhocctlPeerInfoEmu;

typedef struct SceNetAdhocctlPeerInfo {
	struct SceNetAdhocctlPeerInfo * next;
	SceNetAdhocctlNickname nickname;
	SceNetEtherAddr mac_addr;
	uint16_t padding; // unknown
	uint32_t flags;
	uint64_t last_recv;
} __attribute__((packed)) SceNetAdhocctlPeerInfo;

// Game Mode Peer List
#define ADHOCCTL_GAMEMODE_MAX_MEMBERS 16
typedef struct SceNetAdhocctlGameModeInfo {
	int num;
	SceNetEtherAddr member[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
} __attribute__((packed)) SceNetAdhocctlGameModeInfo;

// Adhoc Network Control Handler
typedef void(*SceNetAdhocctlHandler)(int event, int error_code, void * arg);

typedef struct SceUtilityParamBase {
	unsigned int size;
	int message_lang;
	int ctrl_assign;
	int main_thread_priority;
	int sub_thread_priority;
	int font_thread_priority;
	int sound_thread_priority;
	int result; 
	int reserved1;
	int reserved2;
	int reserved3;
	int reserved4;
} __attribute__((packed)) SceUtilityParamBase;

typedef struct SceUtilityNetconfAdhocParam {
	SceNetAdhocctlGroupName group_name;
	uint32_t timeout;
} __attribute__((packed)) SceUtilityNetconfAdhocParam;

typedef struct SceUtilityNetconfParam {
	SceUtilityParamBase base;
	int type;
	SceUtilityNetconfAdhocParam * adhoc_param;
	uint32_t browser_available;
	uint32_t browser_flag;
	uint32_t wifisvc_available;
} __attribute__((packed)) SceUtilityNetconfParam;

#include "../../pspnet_adhocctl_server/packets.h"

#endif
