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

#ifndef _USER_H_
#define _USER_H_

#include <stdint.h>
#include <time.h>
#include <pspstructs.h>
#include <packets.h>

// User States
#define USER_STATE_WAITING 0
#define USER_STATE_LOGGED_IN 1
#define USER_STATE_TIMED_OUT 2

// PSP Resolver Information
typedef struct
{
	// PSP MAC Address
	SceNetEtherAddr mac;
	
	// PSP Hotspot IP Address
	uint32_t ip;
	
	// PSP Player Name
	SceNetAdhocctlNickname name;
} SceNetAdhocctlResolverInfo;

// Type Prototypes
typedef struct SceNetAdhocctlGameNode SceNetAdhocctlGameNode;
typedef struct SceNetAdhocctlGroupNode SceNetAdhocctlGroupNode;

// Double-Linked User List
typedef struct SceNetAdhocctlUserNode {
	// Next Element
	struct SceNetAdhocctlUserNode * next;
	
	// Previous Element
	struct SceNetAdhocctlUserNode * prev;
	
	// Next Element (Group)
	struct SceNetAdhocctlUserNode * group_next;
	
	// Previous Element
	struct SceNetAdhocctlUserNode * group_prev;
	
	// Resolver Information
	SceNetAdhocctlResolverInfo resolver;
	
	// Game Link
	SceNetAdhocctlGameNode * game;
	
	// Group Link
	SceNetAdhocctlGroupNode * group;
	
	// TCP Socket
	int stream;
	
	// Last Ping Update
	time_t last_recv;
	
	// RX Buffer
	uint8_t rx[1024];
	uint32_t rxpos;

	uint16_t port;
} SceNetAdhocctlUserNode;

// Double-Linked Game List
struct SceNetAdhocctlGameNode {
	// Next Element
	struct SceNetAdhocctlGameNode * next;
	
	// Previous Element
	struct SceNetAdhocctlGameNode * prev;
	
	// PSP Game Product Code
	SceNetAdhocctlProductCode game;
	
	// Number of Players
	uint32_t playercount;
	
	// Number of Groups
	uint32_t groupcount;
	
	// Double-Linked Group List
	SceNetAdhocctlGroupNode * group;
};

// Double-Linked Group List
struct SceNetAdhocctlGroupNode {
	// Next Element
	struct SceNetAdhocctlGroupNode * next;
	
	// Previous Element
	struct SceNetAdhocctlGroupNode * prev;
	
	// Game Link
	SceNetAdhocctlGameNode * game;
	
	// PSP Adhoc Group Name
	SceNetAdhocctlGroupName group;
	
	// Number of Players
	uint32_t playercount;
	
	// Double-Linked Player List
	SceNetAdhocctlUserNode * player;
};

// User Count
extern uint32_t _db_user_count;

// User Database
extern SceNetAdhocctlUserNode * _db_user;

// Game Database
extern SceNetAdhocctlGameNode * _db_game;

/**
 * Login User into Database (Stream)
 * @param fd Socket
 * @param ip IP Address (Network Order)
 */
void login_user_stream(int fd, uint32_t ip, uint16_t port);

/**
 * Login User into Database (Login Data)
 * @param user User Node
 * @param data Login Packet
 */
void login_user_data(SceNetAdhocctlUserNode * user, SceNetAdhocctlLoginPacketC2S * data);

/**
 * Logout User from Database
 * @param user User Node
 */
void logout_user(SceNetAdhocctlUserNode * user);

/**
 * Free Database Memory
 */
void free_database(void);

/**
 * Connect User to Game Group
 * @param user User Node
 * @param group Group Name
 */
void connect_user(SceNetAdhocctlUserNode * user, SceNetAdhocctlGroupName * group);

/**
 * Disconnect User from Game Group
 * @param user User Node
 */
void disconnect_user(SceNetAdhocctlUserNode * user);

/**
 * Send Game Group List
 * @param user User Node
 */
void send_scan_results(SceNetAdhocctlUserNode * user);

/**
 * Spread Chat Message in P2P Network
 * @param user Sender User Node
 * @param message Chat Message
 */
void spread_message(SceNetAdhocctlUserNode * user, char * message);

/**
 * Get User State
 * @param user User Node
 */
int get_user_state(SceNetAdhocctlUserNode * user);

/**
 * Clear RX Buffer
 * @param user User Node
 * @param clear Number of Bytes to clear (-1 for all)
 */
void clear_user_rxbuf(SceNetAdhocctlUserNode * user, int clear);

/**
 * Patch Game Product Code
 * @param product To-be-patched Product Code
 * @param from If the Product Code matches this...
 * @param to ... then change it to this one.
 */
void game_product_relink(SceNetAdhocctlProductCode * product, char * from, char * to);

/**
 * Game Product Override (used for mixing multi-region games)
 * @param product IN: Source Product OUT: Override Product
 */
void game_product_override(SceNetAdhocctlProductCode * product);

#endif
