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

#ifndef _ADHOCCTL_FUNCTIONS_H_
#define _ADHOCCTL_FUNCTIONS_H_

/**
 * Read Line from File
 * @param fd File Descriptor to read line from
 * @param buffer Buffer to read line into
 * @param buflen Length of Buffer in Bytes
 * @return Length of Line or... 0
 */
uint32_t _readLine(int fd, char * buffer, uint32_t buflen);


/**
 * Return Number of currently connected Peers
 * @return Number of connected Peers
 */
int _getActivePeerCount(void);

/**
 * Patch utility.prx to allow Kernel Control of Adhoc
 * @param init Replacement for NetconfInit
 * @param getstatus Replacement for NetconfGetStatus
 * @param update Replacement for NetconfUpdate
 * @param shutdown Replacement for NetconfShutdown
 */
void patch_netconf_utility(void * init, void * getstatus, void * update, void * shutdown);

/**
 * Register Incoming Chat-Messages with the Homescreen Chatroom HUD
 * @param nickname Sender Nickname (8 Character Limit)
 * @param text Chat Message (63 Character Limit)
 */
void addChatLog(char * nickname, char * text);

/**
 * Pop to-be-sent Message from Homescreen Chatroom HUD Internal Message Queue
 * @param buffer OUT: Popped Chat Message
 * @return 1 on success or... 0
 */
int popFromOutbox(char * buffer);

/**
 * Set Connection Status in Homescreen Chatroom HUD
 * @param status 1 = Connected, 0 = Disconnected
 */
void setConnectionStatus(int status);

/**
 * Set WLAN Reception for Homescreen HUD
 * @param reception WLAN Reception Percentage
 */
void setReceptionPercentage(int reception);

/**
 * Set User Count in Homescreen Chatroom HUD
 * @param count Number of Users online
 */
void setUserCount(int count);

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
 * Recursive Memory Freeing-Helper for Friend-Structures
 * @param node Current Node in List
 */
void _freeFriendsRecursive(SceNetAdhocctlPeerInfoEmu * node);

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
int _resolveMAC(const SceNetEtherAddr * mac, uint32_t * ip);

/**
 * Get First Peer List Element
 * @return First Internal Peer List Element
 */
SceNetAdhocctlPeerInfoEmu * _getInternalPeerList(void);

int _isMacMatch(const void *lhs, const void *rhs);
int _isMacSelf(SceNetEtherAddr *addr);
void _maccpy(void *dst, const void *src);

void _acquireGroupLock(void);
void _freeGroupLock(void);
void _freeNetworkRecursive(SceNetAdhocctlScanInfo * node);
int search_and_join(const SceNetAdhocctlGroupName *group_name, int timeout_usec);
void _insertGamemodePeer(SceNetEtherAddr *peer);
void _appendGamemodePeer(SceNetEtherAddr *peer);

// hold peer lock before using this
SceNetAdhocctlPeerInfoEmu *_findFriend(void *addr);

int get_gp_value();
int set_gp_value(int gp_value);
void _notifyAdhocctlhandlers(int event, int error_code);

#ifdef DEBUG
#define create_adhocctl_name_buf(new_buf, from) \
	char *new_buf[sizeof(from) + 1] = {0}; \
	memcpy(new_buf, from, sizeof(from));
#else
#define create_adhocctl_name_buf(...)
#endif

#endif
