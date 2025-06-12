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

#ifndef _ADHOC_FUNCTIONS_H_
#define _ADHOC_FUNCTIONS_H_

/**
 * Mersenne Twister Number Generator
 * @param max Maximum Range of Number
 * @return Random unsigned 32bit Number
 */
uint32_t _getRandomNumber(uint32_t max);

/**
 * Closes & Deletes all PDP Sockets
 */
void _deleteAllPDP(void);

/**
 * Closes & Deletes all PTP Sockets
 */
void _deleteAllPTP(void);

/**
 * Deletes all Gamemode Buffer
 */
void _deleteAllGMB(void);

/**
 * Local MAC Check
 * @param saddr To-be-checked MAC Address
 * @return 1 if valid or... 0
 */
int _IsLocalMAC(const SceNetEtherAddr * addr);

/**
 * PDP Port Check
 * @param port To-be-checked Port
 * @return 1 if in use or... 0
 */
int _IsPDPPortInUse(uint16_t port);

/**
 * PDP Socket Counter
 * @return Number of internal PDP Sockets
 */
int _getPDPSocketCount(void);

/**
 * Broadcast MAC Check
 * @param addr To-be-checked MAC Address
 * @return 1 if Broadcast MAC or... 0
 */
int _isBroadcastMAC(const SceNetEtherAddr * addr);

/**
 * Zero MAC check
 * @param addr to be checked
 * @return 1 if mac is made of 0s
 */
int _isZeroMac(const void *addr);

/**
 * PTP Socket Counter
 * @return Number of internal PTP Sockets
 */
int _getPTPSocketCount(void);

/**
 * Check whether PTP Port is in use or not
 * @param sport source port
 * @param listen intended use of the port
 * @param daddr destination mac address
 * @param dport destination port
 * @return 1 if in use or... 0
 */
int _IsPTPPortInUse(uint16_t sport, int listen, const SceNetEtherAddr *daddr, uint16_t dport);

/**
 * Add Port Forward to Router
 * @param protocol "TCP" or "UDP"
 * @param port To-be-forwarded Port Number
 */
void sceNetPortOpen(const char * protocol, uint16_t port);

/**
 * Remove Port Forward from Router
 * @param protocol "TCP" or "UDP"
 * @param port To-be-removed Port Number
 */
void sceNetPortClose(const char * protocol, uint16_t port);

/**
 * Match mac addresses
 * @param lhs
 * @param rhs
 * @return 1 when they are the same
 */
int _isMacMatch(const void *lhs, const void *rhs);

int sceNetAdhocPdpCreate(const SceNetEtherAddr * saddr, uint16_t sport, int bufsize, int flag);
int sceNetAdhocPdpSend(int id, const SceNetEtherAddr * daddr, uint16_t dport, const void * data, int len, uint32_t timeout, int flag);
int sceNetAdhocPdpDelete(int id, int flag);
int sceNetAdhocPdpRecv(int id, SceNetEtherAddr * saddr, uint16_t * sport, void * buf, int * len, uint32_t timeout, int flag);

// From kernel module
void return_memory();
void steal_memory();
#endif
