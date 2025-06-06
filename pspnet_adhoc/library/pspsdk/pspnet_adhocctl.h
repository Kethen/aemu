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

#ifndef _PSPNET_ADHOCCTL_H_
#define _PSPNET_ADHOCCTL_H_

// Adhoc-Control Metaport
#define ADHOCCTL_METAPORT 27312

// Adhoc Nickname
#define ADHOCCTL_NICKNAME_LEN 128
typedef struct SceNetAdhocctlNickname {
	uint8_t data[ADHOCCTL_NICKNAME_LEN];
} __attribute__((packed)) SceNetAdhocctlNickname;

// Peer Information
typedef struct SceNetAdhocctlPeerInfo {
	struct SceNetAdhocctlPeerInfo * next;
	SceNetAdhocctlNickname nickname;
	SceNetEtherAddr mac_addr;
	uint32_t ip_addr;
	uint8_t padding[2];
	uint64_t last_recv;
} __attribute__((packed)) SceNetAdhocctlPeerInfo;

/**
 * Peer Information Getter
 * @param addr Peer MAC Address
 * @param size Size of peer_info (in Bytes)
 * @param peer_info OUT: Peer Information
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG, ADHOC_NO_ENTRY
 */
int sceNetAdhocctlGetPeerInfo(const SceNetEtherAddr * addr, int size, SceNetAdhocctlPeerInfo * peer_info);

/**
 * Peer Information Getter (Linked List)
 * @param buflen IN: Bufferlength OUT: Bufferlength after Output
 * @param buf OUT: Peer Information Linked List
 * @return 0 on success or... ADHOCCTL_NOT_INITIALIZED, ADHOCCTL_INVALID_ARG
 */
int sceNetAdhocctlGetPeerList(int * buflen, SceNetAdhocctlPeerInfo * buf);

/**
 * Lock Peerlist
 */
void _acquirePeerLock(void);

/**
 * Unlock Peerlist
 */
void _freePeerLock(void);

/**
 * Lock Network
 */
void _acquireNetworkLock(void);

/**
 * Unlock Network
 */
void _freeNetworkLock(void);

/**
 * Resolve IP to MAC
 * @param ip Peer IP Address
 * @param mac OUT: Peer MAC
 * @return 0 on success or... ADHOC_NO_ENTRY
 */
int _resolveIP(uint32_t ip, SceNetEtherAddr * mac);

/**
 * Resolve MAC to IP
 * @param mac Peer MAC Address
 * @param ip OUT: Peer IP
 * @return 0 on success or... ADHOC_NO_ENTRY
 */
int _resolveMAC(SceNetEtherAddr * mac, uint32_t * ip);

/**
 * Get First Peer List Element
 * @return First Internal Peer List Element
 */
SceNetAdhocctlPeerInfo * _getInternalPeerList(void);

// some more functions
int sceNetAdhocctlTerm (void);
#endif
