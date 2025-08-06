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

#ifndef _PSPNET_INET_H_
#define _PSPNET_INET_H_

// Protocol Families
#define AF_INET 2

// Socket Types
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

// Protocols
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

// Option Level
#define SOL_SOCKET 0xFFFF

// Option Names
#define SO_REUSEADDR 0x0004
#define SO_KEEPALIVE 0x0008
#define SO_REUSEPORT 0x0200
#define SO_SNDTIMEO 0x1005
#define SO_RCVTIMEO 0x1006
#define SO_TYPE 0x1008
#define SO_NBIO 0x1009
#define SO_SNDBUF 0x1001
#define SO_RCVBUF 0x1002

// IP Macros
#define INADDR_ANY 0

// On-Demand Nonblocking Flag
#define INET_MSG_DONTWAIT 0x80

// Event Flags
#define INET_POLLRDNORM 0x0040
#define INET_POLLWRNORM 0x0004

// Infrastructure Errno Numbers
#define EAGAIN 0x0B
#define ETIMEDOUT 0x74
#define EINPROGRESS 0x77
#define EISCONN 0x7F
#define EALREADY 0x78

// Sockaddr
typedef struct SceNetInetSockaddr {
	uint8_t sa_len;
	uint8_t sa_family;
	uint8_t sa_data[14];
} SceNetInetSockaddr;

// Sockaddr_in
typedef struct SceNetInetSockaddrIn {
	uint8_t sin_len;
	uint8_t sin_family;
	uint16_t sin_port;
	uint32_t sin_addr;
	uint8_t sin_zero[8];
} SceNetInetSockaddrIn;

// Polling Event Field
typedef struct SceNetInetPollfd {
	int fd;
	short events;
	short revents;
} SceNetInetPollfd;

/**
 * Initialize Internet Library
 * @return 0 on success or... error < 0
 */
int sceNetInetInit(void);

/**
 * Terminate Internet Library
 * @return 0 on success or... error < 0
 */
int sceNetInetTerm(void);

/**
 * Create Internet Socket
 * @param domain Protocol Family
 * @param type Socket Type
 * @param protocol Protocol
 * @return Socket ID > 0 on success or... -1
 */
int sceNetInetSocket(int domain, int type, int protocol);

/**
 * Close Internet Socket
 * @param s Socket ID
 * @return 0 on success or... -1
 */
int sceNetInetClose(int s);

/**
 * Set Socket Option
 * @param s Socket ID
 * @param level Option Level
 * @param optname Option Name
 * @param optval Option Value Pointer
 * @param optlen Size of optval (in Bytes)
 * @return 0 on success or... -1
 */
int sceNetInetSetsockopt(int s, int level, int optname, const void * optval, uint32_t optlen);

/**
 * Get Socket Option
 * @param s Socket ID
 * @param level Option Level
 * @param optname Option Name
 * @param optval Option Value Pointer (receives Value)
 * @param optlen IN: Size of optval OUT: Size of optval (can be different) (in Bytes)
 * @return 0 on success or... -1
 */
int sceNetInetGetsockopt(int s, int level, int optname, void * optval, uint32_t * optlen);

/**
 * Bind Socket to local Port
 * @param s Socket ID
 * @param name Address
 * @param namelen Size of name (in Bytes)
 */
int sceNetInetBind(int s, const SceNetInetSockaddr * name, uint32_t namelen);

/**
 * Send Data to Socket-Address
 * @param s Socket ID
 * @param msg Data
 * @param len Size of msg (in Bytes)
 * @param flags Bitflags
 * @param to Target Address
 * @param tolen Size of to (in Bytes)
 * @return Length of sent Data on success or... -1
 */
int sceNetInetSendto(int s, const void * msg, uint32_t len, int flags, const SceNetInetSockaddr * to, uint32_t tolen);

/**
 * Receive Data from unknown Target
 * @param s Socket ID
 * @param buf OUT: Received Data
 * @param len Size of buf (in Bytes)
 * @param flags Bitflags
 * @param from Sender Address
 * @param fromlen OUT: Size of from (in Bytes, used to distinguish between Address Types)
 * @return Length of received Data on success or... -1
 */
int sceNetInetRecvfrom(int s, void * buf, uint32_t len, int flags, SceNetInetSockaddr * from, uint32_t * fromlen);

/**
 * Poll Socket Descriptor Array for Events
 * @param fds Socket ID Set
 * @param nfds Number of Sockets in Array
 * @param timeout Timeout (in Milliseconds)
 * @return Number of Sockets with Events on success or... 0 for Timeout, -1 for Error
 */
int sceNetInetPoll(SceNetInetPollfd * fds, uint32_t nfds, int timeout);

/**
 * Send Data to connected Socket
 * @param s Socket ID
 * @param msg Data
 * @param len Size of msg (in Bytes)
 * @param flags Bitflags
 * @return Length of sent Data on success or... -1
 */
int sceNetInetSend(int s, const void * msg, uint32_t len, int flags);

/**
 * Receive Data from connected Socket
 * @param s Socket ID
 * @param buf OUT: Received Data
 * @param len Size of buf (in Bytes)
 * @param flags Bitflags
 * @return Length of received Data on success or... -1
 */
int sceNetInetRecv(int s, void *buf, uint32_t len, int flags);

/**
 * Switch Socket into Listener Mode
 * @param s Socket ID
 * @param backlog Size of Listening Queue
 * @return 0 on success or... -1
 */
int sceNetInetListen(int s, int backlog);

/**
 * Accept incoming Connection on Listener Socket
 * @param s Socket ID (Listener)
 * @param addr OUT: Accepted Peer Address
 * @param addrlen IN: Size of addr OUT: Size of addr (can be different) (in Bytes)
 * @return Socket ID > 0 on success or... -1
 */
int sceNetInetAccept(int s, SceNetInetSockaddr * addr, uint32_t * addrlen);

/**
 * Connect Socket to Peer
 * @param s Socket ID
 * @param name Peer Address
 * @param namelen Size of name (in Bytes)
 * @return 0 on success or... -1 (-1 can be legit at times, ex. Nonblocking Sockets)
 */
int sceNetInetConnect(int s, const SceNetInetSockaddr * name, uint32_t namelen);

/**
 * Return last produced Error Code
 * @return Errno Error Code
 */
int sceNetInetGetErrno(void);

/**
 * Get remotely connected Peer Information
 * @param s Socket ID
 * @param name OUT: Peer Address
 * @param namelen IN: Size of name OUT: Size of name (can be different) (in Bytes)
 * @return 0 on success or... -1
 */
int sceNetInetGetpeername(int s, SceNetInetSockaddr * name, uint32_t * namelen);

/**
 * Get Local Socket Information
 * @param s Socket ID
 * @param name OUT: Local Address
 * @param namelen IN: Size of name OUT: Size of name (can be different) (in Bytes)
 * @return 0 on success or... -1
 */
int sceNetInetGetsockname(int s, SceNetInetSockaddr * name, uint32_t * namelen);

/**
 * Convert String Display of IPv4 Address to Internal Display
 * @param cp To-be-converted String in IPv4 Address Format
 * @param addr OUT: Internal Display of IP Address
 * @return 1 on success or... 0
 */
int sceNetInetInetAton(const char * cp, uint32_t * addr);

#endif
