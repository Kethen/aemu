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

#ifndef _ADHOC_STRUCTURES_H_
#define _ADHOC_STRUCTURES_H_

#include <stdbool.h>

// Ethernet Address
#define ETHER_ADDR_LEN 6
typedef struct SceNetEtherAddr {
	uint8_t data[ETHER_ADDR_LEN];
} __attribute__((packed)) SceNetEtherAddr;

// Socket Polling Event Listener
typedef struct SceNetAdhocPollSd {
	int id;
	int events;
	int revents;
} __attribute__((packed)) SceNetAdhocPollSd;

// PDP Socket Status
typedef struct SceNetAdhocPdpStat {
	struct SceNetAdhocPdpStat * next;
	int id;
	SceNetEtherAddr laddr;
	uint16_t lport;
	uint32_t rcv_sb_cc;
} __attribute__((packed)) SceNetAdhocPdpStat;

enum PTP_MODES{
	PTP_MODE_LISTEN = 0,
	PTP_MODE_OPEN,
	PTP_MODE_ACCEPT
};

// PTP Socket Status
typedef struct SceNetAdhocPtpStat {
	struct SceNetAdhocPtpStat * next;
	int id;
	SceNetEtherAddr laddr;
	SceNetEtherAddr paddr;
	uint16_t lport;
	uint16_t pport;
	uint32_t snd_sb_cc;
	uint32_t rcv_sb_cc;
	int state;
} __attribute__((packed)) SceNetAdhocPtpStat;

typedef struct PtpSocketExt {
	int mode;
	bool connect_event_fired;
	uint64_t establish_timestamp;
} PtpSocketExt;

// adhoc socket reference
typedef struct AdhocSocket{
	bool is_ptp;
	// cursed, pdp->id is actually not aligned to 4 in this arrangement, so I'm putting this here
	void *postoffice_handle;
	SceUID connect_thread;
	SceNetAdhocPdpStat pdp;
	SceNetAdhocPtpStat ptp;
	PtpSocketExt ptp_ext;
}AdhocSocket;

// Gamemode Optional Peer Buffer Data
typedef struct SceNetAdhocGameModeOptData {
	uint32_t size;
	uint32_t flag;
	uint64_t last_recv;
} __attribute__((packed)) SceNetAdhocGameModeOptData;

// Gamemode Buffer Status
typedef struct SceNetAdhocGameModeBufferStat {
	struct SceNetAdhocGameModeBufferStat * next;
	int id;
	void * ptr;
	uint32_t size;
	uint32_t master;
	SceNetAdhocGameModeOptData opt;
} __attribute__((packed)) SceNetAdhocGameModeBufferStat;

// Current Gamemode
typedef struct GamemodeInternal {
	SceNetEtherAddr saddr;
	void *data;
	uint32_t data_size;
	int data_updated;
	void *recv_buf;
	uint64_t last_recv;
} __attribute__((packed)) GamemodeInternal;

#define ADHOCCTL_GAMEMODE_MAX_MEMBERS 16
typedef struct SceNetAdhocctlGameModeInfo {
	int num;
	SceNetEtherAddr member[ADHOCCTL_GAMEMODE_MAX_MEMBERS];
} __attribute__((packed)) SceNetAdhocctlGameModeInfo;

#endif
