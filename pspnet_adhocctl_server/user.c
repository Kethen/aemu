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

#include <malloc.h>
#include <string.h>
#include <user.h>
#include <status.h>
#include <config.h>
#include <sqlite3.h>

// User Count
uint32_t _db_user_count = 0;

// User Database
SceNetAdhocctlUserNode * _db_user = NULL;

// Game Database
SceNetAdhocctlGameNode * _db_game = NULL;

/**
 * Login User into Database (Stream)
 * @param fd Socket
 * @param ip IP Address (Network Order)
 */
void login_user_stream(int fd, uint32_t ip, uint16_t port)
{
	// Enough Space available
	if(_db_user_count < SERVER_USER_MAXIMUM)
	{
		// Check IP Duplication
		SceNetAdhocctlUserNode * u = _db_user;
		while(u != NULL && (u->resolver.ip != ip || u->port != port)) u = u->next;
		
		// Unique IP Address
		if(u == NULL)
		{
			// Allocate User Node Memory
			SceNetAdhocctlUserNode * user = (SceNetAdhocctlUserNode *)malloc(sizeof(SceNetAdhocctlUserNode));
			
			// Allocated User Node Memory
			if(user != NULL)
			{
				// Clear Memory
				memset(user, 0, sizeof(SceNetAdhocctlUserNode));
				
				// Save Socket
				user->stream = fd;
				
				// Save IP
				user->resolver.ip = ip;

				// Save port
				user->port = port;
				
				// Link into User List
				user->next = _db_user;
				if(_db_user != NULL) _db_user->prev = user;
				_db_user = user;
				
				// Initialize Death Clock
				user->last_recv = time(NULL);
				
				// Notify User
				uint8_t * ipa = (uint8_t *)&user->resolver.ip;
				printf("New Connection from %u.%u.%u.%u.\n", ipa[0], ipa[1], ipa[2], ipa[3]);
				
				// Fix User Counter
				_db_user_count++;
				
				// Update Status Log
				update_status();
				
				// Exit Function
				return;
			}
		}
	}
		
	// Duplicate IP, Allocation Error or not enough space - Close Stream
	close(fd);
}

/**
 * Login User into Database (Login Data)
 * @param user User Node
 * @param data Login Packet
 */
void login_user_data(SceNetAdhocctlUserNode * user, SceNetAdhocctlLoginPacketC2S * data)
{
	// Product Code Check
	int valid_product_code = 1;
	
	// Iterate Characters
	int i = 0; for(; i < PRODUCT_CODE_LENGTH && valid_product_code == 1; i++)
	{
		// Valid Characters
		if(!((data->game.data[i] >= 'A' && data->game.data[i] <= 'Z') || (data->game.data[i] >= '0' && data->game.data[i] <= '9'))) valid_product_code = 0;
	}
	
	// Valid Packet Data
	if(valid_product_code == 1 && memcmp(&data->mac, "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof(data->mac)) != 0 && memcmp(&data->mac, "\x00\x00\x00\x00\x00\x00", sizeof(data->mac)) != 0 && data->name.data[0] != 0)
	{
		// Game Product Override
		game_product_override(&data->game);
		
		// Find existing Game
		SceNetAdhocctlGameNode * game = _db_game;
		while(game != NULL && strncmp(game->game.data, data->game.data, PRODUCT_CODE_LENGTH) != 0) game = game->next;
		
		// Game not found
		if(game == NULL)
		{
			// Allocate Game Node Memory
			game = (SceNetAdhocctlGameNode *)malloc(sizeof(SceNetAdhocctlGameNode));
			
			// Allocated Game Node Memory
			if(game != NULL)
			{
				// Clear Memory
				memset(game, 0, sizeof(SceNetAdhocctlGameNode));
				
				// Save Game Product ID
				game->game = data->game;
				
				// Link into Game List
				game->next = _db_game;
				if(_db_game != NULL) _db_game->prev = game;
				_db_game = game;
			}
		}
		
		// Game now available
		if(game != NULL)
		{
			// Save MAC
			user->resolver.mac = data->mac;
			
			// Save Nickname
			user->resolver.name = data->name;
			
			// Increase Player Count in Game Node
			game->playercount++;
			
			// Link Game to Player
			user->game = game;
			
			// Notify User
			uint8_t * ip = (uint8_t *)&user->resolver.ip;
			char safegamestr[10];
			memset(safegamestr, 0, sizeof(safegamestr));
			strncpy(safegamestr, game->game.data, PRODUCT_CODE_LENGTH);
			printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) started playing %s.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr);
			
			// Update Status Log
			update_status();
			
			// Leave Function
			return;
		}
	}
	
	// Invalid Packet Data
	else
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		printf("Invalid Login Packet Contents from %u.%u.%u.%u.\n", ip[0], ip[1], ip[2], ip[3]);
	}
	
	// Logout User - Out of Memory or Invalid Arguments
	logout_user(user);
}

/**
 * Logout User from Database
 * @param user User Node
 */
void logout_user(SceNetAdhocctlUserNode * user)
{
	// Disconnect from Group
	if(user->group != NULL) disconnect_user(user);

	// Unlink Leftside (Beginning)
	if(user->prev == NULL) _db_user = user->next;
	
	// Unlink Leftside (Other)
	else user->prev->next = user->next;
	
	// Unlink Rightside
	if(user->next != NULL) user->next->prev = user->prev;
	
	// Close Stream
	close(user->stream);
	
	// Playing User
	if(user->game != NULL)
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) stopped playing %s.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr);
		
		// Fix Game Player Count
		user->game->playercount--;
		
		// Empty Game Node
		if(user->game->playercount == 0)
		{
			// Unlink Leftside (Beginning)
			if(user->game->prev == NULL) _db_game = user->game->next;
			
			// Unlink Leftside (Other)
			else user->game->prev->next = user->game->next;
			
			// Unlink Rightside
			if(user->game->next != NULL) user->game->next->prev = user->game->prev;
			
			// Free Game Node Memory
			free(user->game);
		}
	}
	
	// Unidentified User
	else
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		printf("Dropped Connection to %u.%u.%u.%u.\n", ip[0], ip[1], ip[2], ip[3]);
	}
	
	// Free Memory
	free(user);
	
	// Fix User Counter
	_db_user_count--;
	
	// Update Status Log
	update_status();
}

/**
 * Free Database Memory
 */
void free_database(void)
{
	// There are users playing
	if(_db_user_count > 0)
	{
		// Send Shutdown Notice
		spread_message(NULL, SERVER_SHUTDOWN_MESSAGE);
	}
	
	// Iterate Users for Deletion
	SceNetAdhocctlUserNode * user = _db_user;
	while(user != NULL)
	{
		// Next User (for safe delete)
		SceNetAdhocctlUserNode * next = user->next;
		
		// Logout User
		logout_user(user);
		
		// Move Pointer
		user = next;
	}
}

/**
 * Connect User to Game Group
 * @param user User Node
 * @param group Group Name
 */
void connect_user(SceNetAdhocctlUserNode * user, SceNetAdhocctlGroupName * group)
{
	// Group Name Check
	int valid_group_name = 1;
	{
		// Iterate Characters
		int i = 0; for(; i < ADHOCCTL_GROUPNAME_LEN && valid_group_name == 1; i++)
		{
			// End of Name
			if(group->data[i] == 0) break;
			
			// A - Z
			if(group->data[i] >= 'A' && group->data[i] <= 'Z') continue;
			
			// a - z
			if(group->data[i] >= 'a' && group->data[i] <= 'z') continue;
			
			// 0 - 9
			if(group->data[i] >= '0' && group->data[i] <= '9') continue;
			
			// Invalid Symbol
			valid_group_name = 0;
		}
	}
	
	// Valid Group Name
	if(valid_group_name == 1)
	{
		// User is disconnected
		if(user->group == NULL)
		{
			// Find Group in Game Node
			SceNetAdhocctlGroupNode * g = user->game->group;
			while(g != NULL && strncmp((char *)g->group.data, (char *)group->data, ADHOCCTL_GROUPNAME_LEN) != 0) g = g->next;
			
			// BSSID Packet
			SceNetAdhocctlConnectBSSIDPacketS2C bssid;
			
			// Set BSSID Opcode
			bssid.base.opcode = OPCODE_CONNECT_BSSID;
			
			// Set Default BSSID
			bssid.mac = user->resolver.mac;
			
			// No Group found
			if(g == NULL)
			{
				// Allocate Group Memory
				g = (SceNetAdhocctlGroupNode *)malloc(sizeof(SceNetAdhocctlGroupNode));
				
				// Allocated Group Memory
				if(g != NULL)
				{
					// Clear Memory
					memset(g, 0, sizeof(SceNetAdhocctlGroupNode));
					
					// Link Game Node
					g->game = user->game;
					
					// Link Group Node
					g->next = g->game->group;
					if(g->game->group != NULL) g->game->group->prev = g;
					g->game->group = g;
					
					// Copy Group Name
					g->group = *group;
					
					// Increase Group Counter for Game
					g->game->groupcount++;
				}
			}
			
			// Group now available
			if(g != NULL)
			{
				// Iterate remaining Group Players
				SceNetAdhocctlUserNode * peer = g->player;
				while(peer != NULL)
				{
					// Connect Packet
					SceNetAdhocctlConnectPacketS2C packet;
					
					// Clear Memory
					// memset(&packet, 0, sizeof(packet));
					
					// Set Connect Opcode
					packet.base.opcode = OPCODE_CONNECT;
					
					// Set Player Name
					packet.name = user->resolver.name;
					
					// Set Player MAC
					packet.mac = user->resolver.mac;
					
					// Set Player IP
					packet.ip = user->resolver.ip;
					
					// Send Data
					send(peer->stream, &packet, sizeof(packet), 0);
					
					// Set Player Name
					packet.name = peer->resolver.name;
					
					// Set Player MAC
					packet.mac = peer->resolver.mac;
					
					// Set Player IP
					packet.ip = peer->resolver.ip;
					
					// Send Data
					send(user->stream, &packet, sizeof(packet), 0);
					
					// Set BSSID
					if(peer->group_next == NULL) bssid.mac = peer->resolver.mac;
					
					// Move Pointer
					peer = peer->group_next;
				}
				
				// Link User to Group
				user->group_next = g->player;
				if(g->player != NULL) g->player->group_prev = user;
				g->player = user;
				
				// Link Group to User
				user->group = g;
				
				// Increase Player Count
				g->playercount++;
				
				// Send Network BSSID to User
				send(user->stream, &bssid, sizeof(bssid), 0);
				
				// Notify User
				uint8_t * ip = (uint8_t *)&user->resolver.ip;
				char safegamestr[10];
				memset(safegamestr, 0, sizeof(safegamestr));
				strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
				char safegroupstr[9];
				memset(safegroupstr, 0, sizeof(safegroupstr));
				strncpy(safegroupstr, (char *)user->group->group.data, ADHOCCTL_GROUPNAME_LEN);
				printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) joined %s group %s.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr, safegroupstr);

				// Update Status Log
				update_status();
				
				// Exit Function
				return;
			}
		}
		
		// Already connected to another group
		else
		{
			// Notify User
			uint8_t * ip = (uint8_t *)&user->resolver.ip;
			char safegamestr[10];
			memset(safegamestr, 0, sizeof(safegamestr));
			strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
			char safegroupstr[9];
			memset(safegroupstr, 0, sizeof(safegroupstr));
			strncpy(safegroupstr, (char *)group->data, ADHOCCTL_GROUPNAME_LEN);
			char safegroupstr2[9];
			memset(safegroupstr2, 0, sizeof(safegroupstr2));
			strncpy(safegroupstr2, (char *)user->group->group.data, ADHOCCTL_GROUPNAME_LEN);
			printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) attempted to join %s group %s without disconnecting from %s first.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr, safegroupstr, safegroupstr2);
		}
	}
	
	// Invalid Group Name
	else
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		char safegroupstr[9];
		memset(safegroupstr, 0, sizeof(safegroupstr));
		strncpy(safegroupstr, (char *)group->data, ADHOCCTL_GROUPNAME_LEN);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) attempted to join invalid %s group %s.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr, safegroupstr);
	}
	
	// Invalid State, Out of Memory or Invalid Group Name
	logout_user(user);
}

/**
 * Disconnect User from Game Group
 * @param user User Node
 */
void disconnect_user(SceNetAdhocctlUserNode * user)
{
	// User is connected
	if(user->group != NULL)
	{
		// Unlink Leftside (Beginning)
		if(user->group_prev == NULL) user->group->player = user->group_next;
		
		// Unlink Leftside (Other)
		else user->group_prev->group_next = user->group_next;
		
		// Unlink Rightside
		if(user->group_next != NULL) user->group_next->group_prev = user->group_prev;
		
		// Fix Player Count
		user->group->playercount--;
		
		// Iterate remaining Group Players
		SceNetAdhocctlUserNode * peer = user->group->player;
		while(peer != NULL)
		{
			// Disconnect Packet
			SceNetAdhocctlDisconnectPacketS2C packet;
			
			// Clear Memory
			// memset(&packet, 0, sizeof(packet));
			
			// Set Disconnect Opcode
			packet.base.opcode = OPCODE_DISCONNECT;
			
			// Set User IP
			packet.ip = user->resolver.ip;
			
			// Send Data
			send(peer->stream, &packet, sizeof(packet), 0);
			
			// Move Pointer
			peer = peer->group_next;
		}
		
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		char safegroupstr[9];
		memset(safegroupstr, 0, sizeof(safegroupstr));
		strncpy(safegroupstr, (char *)user->group->group.data, ADHOCCTL_GROUPNAME_LEN);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) left %s group %s.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr, safegroupstr);
		
		// Empty Group
		if(user->group->playercount == 0)
		{
			// Unlink Leftside (Beginning)
			if(user->group->prev == NULL) user->group->game->group = user->group->next;
			
			// Unlink Leftside (Other)
			else user->group->prev->next = user->group->next;
			
			// Unlink Rightside
			if(user->group->next != NULL) user->group->next->prev = user->group->prev;
			
			// Free Group Memory
			free(user->group);
			
			// Decrease Group Counter in Game Node
			user->game->groupcount--;
		}
		
		// Unlink from Group
		user->group = NULL;
		user->group_next = NULL;
		user->group_prev = NULL;
		
		// Update Status Log
		update_status();
		
		// Exit Function
		return;
	}
	
	// Not in a game group
	else
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) attempted to leave %s group without joining one first.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr);
	}
	
	// Delete User
	logout_user(user);
}

/**
 * Send Game Group List
 * @param user User Node
 */
void send_scan_results(SceNetAdhocctlUserNode * user)
{
	// User is disconnected
	if(user->group == NULL)
	{
		// Iterate Groups
		SceNetAdhocctlGroupNode * group = user->game->group;
		for(; group != NULL; group = group->next)
		{
			// Scan Result Packet
			SceNetAdhocctlScanPacketS2C packet;
			
			// Clear Memory
			// memset(&packet, 0, sizeof(packet));
			
			// Set Opcode
			packet.base.opcode = OPCODE_SCAN;
			
			// Set Group Name
			packet.group = group->group;
			
			// Iterate Players in Network Group
			SceNetAdhocctlUserNode * peer = group->player;
			for(; peer != NULL; peer = peer->group_next)
			{
				// Found Network Founder
				if(peer->group_next == NULL)
				{
					// Set Group Host MAC
					packet.mac = peer->resolver.mac;
				}
			}
			
			// Send Group Packet
			send(user->stream, &packet, sizeof(packet), 0);
		}
		
		// Notify Player of End of Scan
		uint8_t opcode = OPCODE_SCAN_COMPLETE;
		send(user->stream, &opcode, 1, 0);
		
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) requested information on %d %s groups.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], user->game->groupcount, safegamestr);
		
		// Exit Function
		return;
	}
	
	// User in a game group
	else
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		char safegroupstr[9];
		memset(safegroupstr, 0, sizeof(safegroupstr));
		strncpy(safegroupstr, (char *)user->group->group.data, ADHOCCTL_GROUPNAME_LEN);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) attempted to scan for %s groups without disconnecting from %s first.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr, safegroupstr);
	}
	
	// Delete User
	logout_user(user);
}

/**
 * Spread Chat Message in P2P Network
 * @param user Sender User Node
 * @param message Chat Message
 */
void spread_message(SceNetAdhocctlUserNode * user, char * message)
{
	// Global Notice
	if(user == NULL)
	{
		// Iterate Players
		for(user = _db_user; user != NULL; user = user->next)
		{
			// Player has access to chat
			if(user->group != NULL)
			{
				// Chat Packet
				SceNetAdhocctlChatPacketS2C packet;
				
				// Clear Memory
				memset(&packet, 0, sizeof(packet));
				
				// Set Chat Opcode
				packet.base.base.opcode = OPCODE_CHAT;
				
				// Set Chat Message
				strcpy(packet.base.message, message);
				
				// Send Data
				send(user->stream, &packet, sizeof(packet), 0);
			}
		}
		
		// Prevent NULL Error
		return;
	}
	
	// User is connected
	else if(user->group != NULL)
	{
		// Broadcast Range Counter
		uint32_t counter = 0;
		
		// Iterate Group Players
		SceNetAdhocctlUserNode * peer = user->group->player;
		while(peer != NULL)
		{
			// Skip Self
			if(peer == user)
			{
				// Move Pointer
				peer = peer->group_next;
				
				// Continue Loop
				continue;
			}
			
			// Chat Packet
			SceNetAdhocctlChatPacketS2C packet;
			
			// Set Chat Opcode
			packet.base.base.opcode = OPCODE_CHAT;
			
			// Set Chat Message
			strcpy(packet.base.message, message);
			
			// Set Sender Nickname
			packet.name = user->resolver.name;
			
			// Send Data
			send(peer->stream, &packet, sizeof(packet), 0);
			
			// Move Pointer
			peer = peer->group_next;
			
			// Increase Broadcast Range Counter
			counter++;
		}
		
		// Message Sent
		if(counter > 0)
		{
			// Notify User
			uint8_t * ip = (uint8_t *)&user->resolver.ip;
			char safegamestr[10];
			memset(safegamestr, 0, sizeof(safegamestr));
			strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
			char safegroupstr[9];
			memset(safegroupstr, 0, sizeof(safegroupstr));
			strncpy(safegroupstr, (char *)user->group->group.data, ADHOCCTL_GROUPNAME_LEN);
			printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) sent \"%s\" to %d players in %s group %s.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], message, counter, safegamestr, safegroupstr);
		}
		
		// Exit Function
		return;
	}
	
	// User not in a game group
	else
	{
		// Notify User
		uint8_t * ip = (uint8_t *)&user->resolver.ip;
		char safegamestr[10];
		memset(safegamestr, 0, sizeof(safegamestr));
		strncpy(safegamestr, user->game->game.data, PRODUCT_CODE_LENGTH);
		printf("%s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP: %u.%u.%u.%u) attempted to send a text message without joining a %s group first.\n", (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip[0], ip[1], ip[2], ip[3], safegamestr);
	}
	
	// Delete User
	logout_user(user);
}

/**
 * Get User State
 * @param user User Node
 */
int get_user_state(SceNetAdhocctlUserNode * user)
{
	// Timeout Status
	if((time(NULL) - user->last_recv) >= SERVER_USER_TIMEOUT) return USER_STATE_TIMED_OUT;
	
	// Waiting Status
	if(user->game == NULL) return USER_STATE_WAITING;
	
	// Logged-In Status
	return USER_STATE_LOGGED_IN;
}

/**
 * Clear RX Buffer
 * @param user User Node
 * @param clear Number of Bytes to clear (-1 for all)
 */
void clear_user_rxbuf(SceNetAdhocctlUserNode * user, int clear)
{
	// Fix Clear Length
	if(clear == -1 || clear > user->rxpos) clear = user->rxpos;
	
	// Move Buffer
	memmove(user->rx, user->rx + clear, sizeof(user->rx) - clear);
	
	// Fix RX Buffer Pointer
	user->rxpos -= clear;
}

/**
 * Patch Game Product Code
 * @param product To-be-patched Product Code
 * @param from If the Product Code matches this...
 * @param to ... then change it to this one.
 */
void game_product_relink(SceNetAdhocctlProductCode * product, char * from, char * to)
{
	// Relink Region Code
	if(strncmp(product->data, from, PRODUCT_CODE_LENGTH) == 0) strncpy(product->data, to, PRODUCT_CODE_LENGTH);
}

/**
 * Game Product Override (used for mixing multi-region games)
 * @param product IN: Source Product OUT: Override Product
 */
void game_product_override(SceNetAdhocctlProductCode * product)
{
	// Safe Product Code
	char productid[PRODUCT_CODE_LENGTH + 1];
	
	// Prepare Safe Product Code
	strncpy(productid, product->data, PRODUCT_CODE_LENGTH);
	productid[PRODUCT_CODE_LENGTH] = 0;
	
	// Database Handle
	sqlite3 * db = NULL;
	
	// Open Database
	if(sqlite3_open(SERVER_DATABASE, &db) == SQLITE_OK)
	{
		// Crosslinked Flag
		int crosslinked = 0;
		
		// Exists Flag
		int exists = 0;
		
		// SQL Statements
		const char * sql = "SELECT id_to FROM crosslinks WHERE id_from=?;";
		const char * sql2 = "SELECT * FROM productids WHERE id=?;";
		const char * sql3 = "INSERT INTO productids(id, name) VALUES(?, ?);";
		
		// Prepared SQL Statement
		sqlite3_stmt * statement = NULL;
		
		// Prepare SQL Statement
		if(sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &statement, NULL) == SQLITE_OK)
		{
			// Bind SQL Statement Data
			if(sqlite3_bind_text(statement, 1, productid, strlen(productid), SQLITE_STATIC) == SQLITE_OK)
			{
				// Found Matching Row
				if(sqlite3_step(statement) == SQLITE_ROW)
				{
					// Grab Crosslink ID
					const char * crosslink = (const char *)sqlite3_column_text(statement, 0);
					
					// Crosslink Product Code
					strncpy(product->data, crosslink, PRODUCT_CODE_LENGTH);
					
					// Log Crosslink
					printf("Crosslinked %s to %s.\n", productid, crosslink);
					
					// Set Crosslinked Flag
					crosslinked = 1;
				}
			}
			
			// Destroy Prepared SQL Statement
			sqlite3_finalize(statement);
		}
		
		// Not Crosslinked
		if(!crosslinked)
		{
			// Prepare SQL Statement
			if(sqlite3_prepare_v2(db, sql2, strlen(sql2) + 1, &statement, NULL) == SQLITE_OK)
			{
				// Bind SQL Statement Data
				if(sqlite3_bind_text(statement, 1, productid, strlen(productid), SQLITE_STATIC) == SQLITE_OK)
				{
					// Found Matching Row
					if(sqlite3_step(statement) == SQLITE_ROW)
					{
						// Set Exists Flag
						exists = 1;
					}
				}
				
				// Destroy Prepare SQL Statement
				sqlite3_finalize(statement);
			}
			
			// Game doesn't exist in Database
			if(!exists)
			{
				// Prepare SQL Statement
				if(sqlite3_prepare_v2(db, sql3, strlen(sql3) + 1, &statement, NULL) == SQLITE_OK)
				{
					// Bind SQL Statement Data
					if(sqlite3_bind_text(statement, 1, productid, strlen(productid), SQLITE_STATIC) == SQLITE_OK && sqlite3_bind_text(statement, 2, productid, strlen(productid), SQLITE_STATIC) == SQLITE_OK)
					{
						// Save Product ID to Database
						if(sqlite3_step(statement) == SQLITE_DONE)
						{
							// Log Addition
							printf("Added Unknown Product ID %s to Database.\n", productid);
						}
					}
					
					// Destroy Prepare SQL Statement
					sqlite3_finalize(statement);
				}
			}
		}
		
		// Close Database
		sqlite3_close(db);
	}
}

