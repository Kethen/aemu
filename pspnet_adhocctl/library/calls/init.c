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

#include "../common.h"
#include <ctype.h>

// Library State
int _init = 0;

// Thread Status
int _thread_status = ADHOCCTL_STATE_DISCONNECTED;

// UPNP Library Handle
int _upnp_uid = -1;
int _upnp_start_status = -1;

// Game Product Code
SceNetAdhocctlAdhocId _product_code;

// Game Group
SceNetAdhocctlParameter _parameter;

// Peer List
SceNetAdhocctlPeerInfoEmu * _friends = NULL;

// Scan Network List
SceNetAdhocctlScanInfo * _networks = NULL;
SceNetAdhocctlScanInfo * _newnetworks = NULL;


// Event Handler
SceNetAdhocctlHandler _event_handler[ADHOCCTL_MAX_HANDLER];
int _event_handler_gp[ADHOCCTL_MAX_HANDLER];
void * _event_args[ADHOCCTL_MAX_HANDLER];

// Access Point Setting Name
int _hotspot = -1;

// Meta Socket
int _metasocket = -1;

#ifdef ENABLE_PEERLOCK
// Peer Locker
int _peerlock = 0;
#endif

#ifdef ENABLE_NETLOCK
// Network Locker
int _networklock = 0;
#endif

// Bit-Values
int _one = 1;
int _zero = 0;


// Function Prototypes
int _initNetwork(const SceNetAdhocctlAdhocId * adhoc_id);
int _readHotspotConfig(void);
int _findHotspotConfigId(char * ssid);
const char * _readServerConfig(void);
void _readChatKeyphrases(const SceNetAdhocctlAdhocId * adhoc_id);
int _friendFinder(SceSize args, void * argp);
void _addFriend(SceNetAdhocctlConnectPacketS2C * packet);
void _deleteFriendByIP(uint32_t ip);

#pragma GCC push_options
#pragma GCC optimize ("O0")
int get_gp_value()
{
	register int gp asm ("gp");
	int gp_value = gp;
	return gp_value;
}

int set_gp_value(int gp_value)
{
	register int gp asm ("gp");
	gp = gp_value;
	return 0;
}
#pragma GCC pop_options

static const char *get_adhocctl_event_name(int event){
	switch(event){
		case ADHOCCTL_EVENT_ERROR:
			return "ADHOCCTL_EVENT_ERROR";
		case ADHOCCTL_EVENT_CONNECT:
			return "ADHOCCTL_EVENT_CONNECT";
		case ADHOCCTL_EVENT_DISCONNECT:
			return "ADHOCCTL_EVENT_DISCONNECT";
		case ADHOCCTL_EVENT_SCAN:
			return "ADHOCCTL_EVENT_SCAN";
		case ADHOCCTL_EVENT_GAMEMODE:
			return "ADHOCCTL_EVENT_GAMEMODE";
	}
	return "unknown adhoc event";
}

void _notifyAdhocctlhandlers(int event, int error_code)
{
	printk("%s: event 0x%x(%s) 0x%x\n", __func__, event, get_adhocctl_event_name(event), error_code);
	int i = 0; for(; i < ADHOCCTL_MAX_HANDLER; i++)
	{
		// Active Handler
		if(_event_handler[i] == NULL)
		{
			continue;
		}

		int old_gp_value = get_gp_value();

		printk("%s: handler 0x%x 0x%x local gp 0x%x game gp 0x%x free stack 0x%x\n", __func__, _event_handler[i], _event_args[i], old_gp_value, _event_handler_gp[i], sceKernelGetThreadStackFreeSize(0));

		set_gp_value(_event_handler_gp[i]);
		_event_handler[i](event, error_code, _event_args[i]);
		set_gp_value(old_gp_value);
	}
}

static struct SceKernelThreadOptParam thread_px_stack_opt = {
	.size = sizeof(struct SceKernelThreadOptParam),
	.stackMpid = 5,
};

int partition_to_use();

/**
 * Initialize the Adhoc-Control Emulator
 * @param stacksize Stacksize of the Internal Thread
 * @param prio Internal Thread Priority
 * @param adhoc_id Game Product Code
 * @return 0 on success or... ADHOCCTL_ALREADY_INITIALIZED, ADHOCCTL_STACKSIZE_TOO_SHORT, ADHOCCTL_INVALID_ARG
 */
int proNetAdhocctlInit(int stacksize, int prio, const SceNetAdhocctlAdhocId * adhoc_id)
{
	// Uninitialized Library
	if(_init == 0)
	{
		// Minimum Stacksize (just to fake SCE behaviour)
		if(stacksize >= 3072)
		{
			if (_readHotspotConfig() != 0)
			{
				printk("%s: failed reading hotspot config\n", __func__);
				return -1;
			}

			// Get Server IP (String)
			const char * ip = _readServerConfig();

			if (ip == NULL)
			{
				printk("%s: failed reading ip config\n", __func__);
				return -1;
			}

			if (_initNetwork(adhoc_id) != 0)
			{
				printk("%s: failed initializing networking\n", __func__);
				return -1;
			}

			// Create Main Thread
			thread_px_stack_opt.stackMpid = partition_to_use();
			int update = sceKernelCreateThread("friend_finder", _friendFinder, prio, 32768, 0, &thread_px_stack_opt);
						
			if (update < 0)
			{
				printk("%s: failed creating friend finder thread\n", __func__, update);
				return -1;
			}

			// Initialization Success
			_init = 1;
			
			// Start Main Thread
			sceKernelStartThread(update, 0, NULL);

			// Return Success
			return 0;
		}
		
		// Stack too small
		return ADHOCCTL_STACKSIZE_TOO_SHORT;
	}
	
	// Initialized Library
	return ADHOCCTL_ALREADY_INITIALIZED;
}

void miniupnc_start();

// implemented in pspnet_adhoc
int resolve_server_ip();

void apctl_disconnect_and_wait_till_disconnected(){
	sceNetApctlDisconnect();
	int state = 4;
	while(state == 4){
		sceKernelDelayThread(100000);
		int get_result = sceNetApctlGetState(&state);
		if (get_result != 0){
			printk("%s: failed getting apctl state, 0x%x\n", __func__, get_result);
			break;
		}
	}
	printk("%s: returning on state %d\n", __func__, state);
}

/**
 * Initialize Networking Components for Adhocctl Emulator
 * @param adhoc_id Game Product Code
 * @param server_ip Server IP
 * @return 0 on success or... -1
 */
int _initNetwork(const SceNetAdhocctlAdhocId * adhoc_id)
{
	#if 0
	// WLAN Switch Check
	int wlan_switch_state = sceWlanGetSwitchState();
	if (wlan_switch_state != 1)
	{
		printk("%s: wlan switch is off\n", __func__);
		return -1;
	}

	int apctl_init_status = sceNetApctlInit(0x1800, 0x30);
	if (apctl_init_status != 0){
		printk("%s: sceNetApctlInit failed, 0x%x\n", __func__, apctl_init_status);
		return -1;
	}
	#endif

	// Attempt Counter
	int attemptmax = 20;

	// Attempt Number
	int attempt = 0;

	// Attempt Connection Setup
	for(; attempt < attemptmax; attempt++)
	{
		#if 0
		int apctl_connect_status = sceNetApctlConnect(_hotspot);
		if (apctl_connect_status != 0)
		{
			printk("%s: sceNetApctlConnect failed on attempt %d, 0x%x\n", __func__, attempt, apctl_connect_status);
			continue;
		}

		// Wait for Connection
		int statebefore = 0;
		int state = 0; while(state != 4)
		{
			// Query State
			int getstate = sceNetApctlGetState(&state);
			
			// Log State Change
			if(statebefore != state) printk("New Connection State: %d\n", state);
			
			// Query Success
			if(getstate == 0 && state != 4)
			{
				// Wait for Retry
				sceKernelDelayThread(1000000);
			}
			
			// Query Error
			else
			{
				printk("%s: sceNetApctlGetState returned 0x%x\n", __func__, getstate);
				break;
			}

			if (state == 0 && statebefore != 0){
				printk("%s: sceNetApctlGetState got disconnect state\n", __func__);
				break;
			}
			
			// Save Before State
			statebefore = state;
		}

		if (state != 4)
		{
			printk("%s: failed connecting to ap on attempt %d, state %d\n", __func__, attempt, state);
			// Close Hotspot Connection
			apctl_disconnect_and_wait_till_disconnected();
			continue;
		}
		#else
		int wait_secs = 0;
		while(!_wifi_connected && wait_secs < 10){
			printk("%s: waiting for wifi connection to be established, please check your hotspot.txt, it should have been connected by now\n", __func__);
			sceKernelDelayThread(1000000);
			wait_secs++;
		}
		if (!_wifi_connected){
			continue;
		}
		#endif

		// Create Friend Finder Socket
		int socket = sceNetInetSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (socket <= 0)
		{
			printk("%s: failed creating internet socket on attempt %d, 0x%x\n", __func__, attempt, socket);
			// Close Hotspot Connection
			apctl_disconnect_and_wait_till_disconnected();
			continue;
		}

		// Enable Port Re-use
		sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &_one, sizeof(_one));
		sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &_one, sizeof(_one));
		
		// Apply Receive Timeout Settings to Socket
		// uint32_t timeout = ADHOCCTL_RECV_TIMEOUT;
		// sceNetInetSetsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		// PSVita 1000 seems to have issues connecting right after adhocctl
		sceKernelDelayThread(100000 * attempt);

		// Server IP
		uint32_t ip = resolve_server_ip();
		if (ip == 0xFFFFFFFF){
			printk("%s: failed resolving server ip address on attempt %d\n", __func__, attempt);
			apctl_disconnect_and_wait_till_disconnected();
			continue;
		}

		// Prepare Server Address
		SceNetInetSockaddrIn addr;
		addr.sin_len = sizeof(addr);
		addr.sin_family = AF_INET;
		addr.sin_addr = ip;
		addr.sin_port = sceNetHtons(ADHOCCTL_METAPORT);

		int connect_status = sceNetInetConnect(socket, (SceNetInetSockaddr *)&addr, sizeof(addr));
		if (connect_status != 0)
		{
			printk("%s: failed connecting to server 0x%x on attempt %d, 0x%x\n", __func__, ip, attempt, connect_status);
			// Delete Socket
			sceNetInetClose(socket);
			// Close Hotspot Connection
			apctl_disconnect_and_wait_till_disconnected();
			continue;
		}
		
		// Save Meta Socket
		_metasocket = socket;
		
		// Save Product Code
		_product_code = *adhoc_id;
		
		// Clear Event Handler
		memset(_event_handler, 0, sizeof(_event_handler[0]) * ADHOCCTL_MAX_HANDLER);
		memset(_event_args, 0, sizeof(_event_args[0]) * ADHOCCTL_MAX_HANDLER);
		
		// Clear Internal Control Status
		memset(&_parameter, 0, sizeof(_parameter));
		
		// Read PSP Player Name
		sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, (char *)_parameter.nickname.data, ADHOCCTL_NICKNAME_LEN);
		
		#if 0
		// Read Adhoc Channel
		sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_ADHOC_CHANNEL, &_parameter.channel);
		
		// Fake Channel Number 1 on Automatic Channel
		if(_parameter.channel == 0) _parameter.channel = 1;
		#else
		// Forcing channel 1 for now
		printk("%s: forcing channel 1 for now\n", __func__);
		_parameter.channel = 1;
		#endif

		// Read PSP MAC Address
		sceWlanGetEtherAddr((void *)&_parameter.bssid.mac_addr.data);
		
		// Prepare Login Packet
		SceNetAdhocctlLoginPacketC2S packet;
		
		// Set Packet Opcode
		packet.base.opcode = OPCODE_LOGIN;
		
		// Set MAC Address
		packet.mac = _parameter.bssid.mac_addr;
		
		// Set Nickname
		packet.name = _parameter.nickname;
		
		// Set Game Product ID
		memcpy(packet.game.data, adhoc_id->data, ADHOCCTL_ADHOCID_LEN);
		
		// Acquire Network Layer Lock
		_acquireNetworkLock();
		
		// Send Login Packet
		sceNetInetSend(_metasocket, &packet, sizeof(packet), INET_MSG_DONTWAIT);
		
		// Free Network Layer Lock
		_freeNetworkLock();
		
		// Load and start UPNP Library, optional
		if (_upnp_uid < 0)
			_upnp_uid = sceKernelLoadModule("ms0:/kd/pspnet_miniupnc.prx", 0, NULL);

		int _upnp_start_status = -1;
		if (_upnp_uid >= 0 && _upnp_start_status < 0)
		{
			int status;
			_upnp_start_status = sceKernelStartModule(_upnp_uid, 0, NULL, &status, NULL);
		}
		else
		{
			printk("%s: failed loading upnp module, 0x%x\n", __func__, _upnp_uid);
		}

		if (_upnp_start_status < 0)
		{
			printk("%s: failed starting upnp module, 0x%x\n", __func__, _upnp_start_status);
		}

		// Best effort
		miniupnc_start();

		// Return Success
		return 0;
	}

	// Terminate Access Point Control
	sceNetApctlTerm();

	// Generic Error
	return -1;
}

/**
 * Read Access Point Configuration Name
 * @return 0 on success or... -1
 */
int _readHotspotConfig(void)
{
	// Open Configuration File
	int fd = sceIoOpen("ms0:/seplugins/hotspot.txt", PSP_O_RDONLY, 0777);
	
	// Opened Configuration File
	if(fd >= 0)
	{
		// Line Buffer
		char line[128];
		
		// Read Line
		_readLine(fd, line, sizeof(line));
		
		// Find Hotspot Configuration
		_hotspot = _findHotspotConfigId(line);
		
		// Found Hotspot
		if(_hotspot != -1) printk("Selected Hotspot: %s\n", line);
		
		// No Hotspot found
		else printk("Couldn't find Hotspot: %s\n", line);
		
		// Close Configuration File
		sceIoClose(fd);
		
		// Return Success
		if(_hotspot >= 0) return 0;
	}
	
	// Generic Error
	return -1;
}

/**
 * Find Infrastructure Configuration ID for SSID
 * @param ssid Target SSID
 * @return Configuration ID > -1 or... -1
 */
int _findHotspotConfigId(char * ssid)
{
	// Counter
	int i = 0;
	
	// Find Hotspot by SSID
	for(; i <= 10; i++)
	{
		// Parameter Container
		netData entry;
		
		// Acquire SSID for Configuration
		if(sceUtilityGetNetParam(i, PSP_NETPARAM_SSID, &entry) == 0)
		{
			// Log Parameter
			printk("Reading PSP Infrastructure Profile for %s\n", entry.asString);
			
			// Hotspot Configuration found
			if(strcmp(entry.asString, ssid) == 0) return i;
		}
	}

	printk("%s: Hotspot with SSID %s not found, using config 0\n", __func__, ssid);
	return 0;
}

/**
 * Read Server IP
 * @return IP != 0 on success or... 0
 */
const char * _readServerConfig(void)
{
	// Line Buffer
	static char line[128];

	// Open Configuration File
	int fd = sceIoOpen("ms0:/seplugins/server.txt", PSP_O_RDONLY, 0777);
	
	// Opened Configuration File
	if(fd >= 0)
	{
		// Read Line
		_readLine(fd, line, sizeof(line));
		
		// Close Configuration File
		sceIoClose(fd);
		
		// Return IP (String)
		return line;
	}
	
	// Generic Error
	return NULL;
}

/**
 * Read Line from File
 * @param fd File Descriptor to read line from
 * @param buffer Buffer to read line into
 * @param buflen Length of Buffer in Bytes
 * @return Length of Line or... 0
 */
uint32_t _readLine(int fd, char * buffer, uint32_t buflen)
{
	// Erase Buffer
	memset(buffer, 0, buflen);
	
	// Length of Line
	uint32_t length = 0;
	
	// Read Line
	while(length < buflen - 1)
	{
		// Read Character
		int b = sceIoRead(fd, buffer + length, 1);
		
		// Read Failure
		if(b <= 0) break;
		
		// Move Pointer
		length++;		
		
		// End of Line Symbol
		if(buffer[length - 1] == '\n') break;
	}
	
	// Remove End of Line Symbols
	int i = 0; for(; i < length; i++) if(buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = 0;
	
	// Return true Length
	return strlen(buffer);
}

#if 0
static int stricmp(const char *lhs, const char *rhs)
{
	int lhs_len = strlen(lhs);
	int rhs_len = strlen(rhs);
	if (lhs_len > 511 || rhs_len > 511)
	{
		// giving up, nametags shoud not be longer than 512
		return 0;
	}
	char buf_lhs[512];
	char buf_rhs[512];

	#define TO_LOWER(dst, src, len) { \
		dst[len] = 0; \
		for(int _i = 0;_i < len;_i++) { \
			dst[_i] = tolower(src[_i]); \
		} \
	}
	TO_LOWER(buf_lhs, lhs, lhs_len);
	TO_LOWER(buf_rhs, rhs, rhs_len);
	#undef TO_LOWER
	return strcmp(buf_lhs, buf_rhs);
}
#endif

/**
 * Friend Finder Thread (Receives Peer Information)
 * @param args Length of argp in Bytes (Unused)
 * @param argp Argument (Unused)
 * @return Unused Value - Return 0
 */
int _friendFinder(SceSize args, void * argp)
{
	// Receive Buffer
	int rxpos = 0;
	uint8_t rx[1024];
	
	// Chat Packet
	SceNetAdhocctlChatPacketC2S chat;
	chat.base.opcode = OPCODE_CHAT;
	
	// Last Ping Time
	uint64_t lastping = 0;
	
	// Last Time Reception got updated
	uint64_t lastreceptionupdate = 0;
	
	// Finder Loop
	while(_init == 1)
	{
		// Acquire Network Lock
		_acquireNetworkLock();
		
		// Ping Server
		if((sceKernelGetSystemTimeWide() - lastping) >= ADHOCCTL_PING_TIMEOUT)
		{
			// Update Ping Time
			lastping = sceKernelGetSystemTimeWide();
			
			// Prepare Packet
			uint8_t opcode = OPCODE_PING;
			
			// Send Ping to Server
			sceNetInetSend(_metasocket, &opcode, 1, INET_MSG_DONTWAIT);
		}
		
		// Send Chat Messages
		while(popFromOutbox(chat.message))
		{
			// Send Chat to Server
			sceNetInetSend(_metasocket, &chat, sizeof(chat), INET_MSG_DONTWAIT);
		}

		// Wait for Incoming Data
		int received = sceNetInetRecv(_metasocket, rx + rxpos, sizeof(rx) - rxpos, INET_MSG_DONTWAIT);

		// Free Network Lock
		_freeNetworkLock();

		// Some games cannot handle the disconnect signal emitted in the disconnect call
		// half a second
		if (_disconnect_timestamp != 0 && sceKernelGetSystemTimeWide() - _disconnect_timestamp > 500000)
		{
			_disconnect_timestamp = 0;
			_notifyAdhocctlhandlers(ADHOCCTL_EVENT_DISCONNECT, 0);
		}

		// 10 seconds timeout for joining gamemode
		if (_thread_status == ADHOCCTL_STATE_CONNECTED &&
			_in_gamemode == -1 &&
			_gamemode_join_timestamp != 0 &&
			sceKernelGetSystemTimeWide() - _gamemode_join_timestamp > 10000000
		)
		{
			// Trigger disconnect
			printk("%s: gamemode joining timed out\n", __func__);
			for (int i = 0;i < _num_actual_gamemode_peers;i++)
			{
				printk("%s: member %x:%x:%x:%x:%x:%x\n", __func__, _actual_gamemode_peers[i].data[0], _actual_gamemode_peers[i].data[1], _actual_gamemode_peers[i].data[2], _actual_gamemode_peers[i].data[3], _actual_gamemode_peers[i].data[4], _actual_gamemode_peers[i].data[5]);
			}
			_notifyAdhocctlhandlers(ADHOCCTL_EVENT_ERROR, ERROR_NET_ADHOC_TIMEOUT);
			_in_gamemode = 0;
			//proNetAdhocctlExitGameMode();
		}

		// Received Data
		if(received > 0)
		{
			// Fix Position
			rxpos += received;
			
			// Log Incoming Traffic
			#ifdef DEBUG
			printk("Received %d Bytes of Data from Server\n", received);
			#endif
		}
		
		// Handle Packets
		if(rxpos > 0)
		{
			// BSSID Packet
			if(rx[0] == OPCODE_CONNECT_BSSID)
			{
				// Enough Data available
				if(rxpos >= sizeof(SceNetAdhocctlConnectBSSIDPacketS2C))
				{
					// Cast Packet
					SceNetAdhocctlConnectBSSIDPacketS2C * packet = (SceNetAdhocctlConnectBSSIDPacketS2C *)rx;
					
					// Update BSSID
					_parameter.bssid.mac_addr = packet->mac;
					
					// Change State
					_thread_status = ADHOCCTL_STATE_CONNECTED;

					printk("%s: OPCODE_CONNECT_BSSID with address %x:%x:%x:%x:%x:%x\n", __func__, packet->mac.data[0], packet->mac.data[1], packet->mac.data[2], packet->mac.data[3], packet->mac.data[4], packet->mac.data[5]);

					// Notify Event Handlers
					if (!_in_gamemode)
					{
						// add a delay here, in case game checks the friend list immediately after a room switch
						// this delay has to be smaller than the delay of ADHOCCTL_EVENT_CONNECT wait in netconf update
						sceKernelDelayThread(100000);
						_notifyAdhocctlhandlers(ADHOCCTL_EVENT_CONNECT, 0);
					}
					else
					{
						// PPSSPP inserts self here into gamemode member list
						_gamemode_self_arrived = 1;
						// Should we assume host arrived as well, or should we check the mac address here..? In theory the BSSID is owned by the host...
						_gamemode_host_arrived = 1;

						if (_num_actual_gamemode_peers >= _num_gamemode_peers && _gamemode_self_arrived && _gamemode_host_arrived)
						{
							_in_gamemode = 1;
							_notifyAdhocctlhandlers(ADHOCCTL_EVENT_GAMEMODE, 0);
							asm volatile("": : :"memory");
							_gamemode_notified = 1;
							printk("%s: sent gamemode event from OPCODE_CONNECT_BSSID handler with %d member(s), expected member(s) %d\n", __func__, _num_actual_gamemode_peers, _num_gamemode_peers);
							#ifdef DEBUG
							for (int i = 0;i < _num_actual_gamemode_peers;i++)
							{
								printk("%s: member %x:%x:%x:%x:%x:%x\n", __func__, _actual_gamemode_peers[i].data[0], _actual_gamemode_peers[i].data[1], _actual_gamemode_peers[i].data[2], _actual_gamemode_peers[i].data[3], _actual_gamemode_peers[i].data[4], _actual_gamemode_peers[i].data[5]);
							}
							#endif
						}
					}
					
					// Move RX Buffer
					memmove(rx, rx + sizeof(SceNetAdhocctlConnectBSSIDPacketS2C), sizeof(rx) - sizeof(SceNetAdhocctlConnectBSSIDPacketS2C));
					
					// Fix RX Buffer Length
					rxpos -= sizeof(SceNetAdhocctlConnectBSSIDPacketS2C);
				}
			}
			
			// Chat Packet
			else if(rx[0] == OPCODE_CHAT)
			{
				// Enough Data available
				if(rxpos >= sizeof(SceNetAdhocctlChatPacketS2C))
				{
					// Cast Packet
					SceNetAdhocctlChatPacketS2C * packet = (SceNetAdhocctlChatPacketS2C *)rx;
					
					// what is the ME name tag? I need a history lesson on it... disabling for now
					#if 0
					// Fix for Idiots that try to troll the "ME" Nametag
					if(stricmp((char *)packet->name.data, "ME") == 0) strcpy((char *)packet->name.data, "NOT ME");
					#endif
					
					// Add Incoming Chat to HUD
					addChatLog((char *)packet->name.data, packet->base.message);
					
					// Move RX Buffer
					memmove(rx, rx + sizeof(SceNetAdhocctlChatPacketS2C), sizeof(rx) - sizeof(SceNetAdhocctlChatPacketS2C));
					
					// Fix RX Buffer Length
					rxpos -= sizeof(SceNetAdhocctlChatPacketS2C);
				}
			}
			
			// Connect Packet
			else if(rx[0] == OPCODE_CONNECT)
			{
				// Enough Data available
				if(rxpos >= sizeof(SceNetAdhocctlConnectPacketS2C))
				{
					// Log Incoming Peer
					#ifdef DEBUG
					printk("Incoming Peer Data...\n");
					#endif
					
					// Cast Packet
					SceNetAdhocctlConnectPacketS2C * packet = (SceNetAdhocctlConnectPacketS2C *)rx;
					
					// Add User
					_addFriend(packet);
					
					// Update HUD User Count
					#ifdef LOCALHOST_AS_PEER
					setUserCount(_getActivePeerCount());
					#else
					setUserCount(_getActivePeerCount()+1);
					#endif

					printk("%s: OPCODE_CONNECT with address %x:%x:%x:%x:%x:%x\n", __func__, packet->mac.data[0], packet->mac.data[1], packet->mac.data[2], packet->mac.data[3], packet->mac.data[4], packet->mac.data[5]);

					// Game mode notify
					if (_in_gamemode)
					{
						// Odd cases where we receive self/host here...
						int is_self = _isMacSelf(&packet->mac);
						int is_host = _isMacMatch(&packet->mac, &_gamemode_host);

						if (is_self)
						{
							_gamemode_self_arrived = 1;
						}
						if (is_host)
						{
							_gamemode_host_arrived = 1;
						}
						if (!is_self && !is_host)
						{
							// Insert
							//_insertGamemodePeer(&packet->mac);
							_appendGamemodePeer(&packet->mac);
						}

						if (_num_actual_gamemode_peers >= _num_gamemode_peers && _gamemode_self_arrived && _gamemode_host_arrived)
						{
							_in_gamemode = 1;
							_notifyAdhocctlhandlers(ADHOCCTL_EVENT_GAMEMODE, 0);
							asm volatile("": : :"memory");
							_gamemode_notified = 1;
							printk("%s: sent gamemode event from OPCODE_CONNECT handler with %d member(s), expected member(s) %d\n", __func__, _num_actual_gamemode_peers, _num_gamemode_peers);
							#ifdef DEBUG
							for (int i = 0;i < _num_actual_gamemode_peers;i++)
							{
								printk("%s: member %x:%x:%x:%x:%x:%x\n", __func__, _actual_gamemode_peers[i].data[0], _actual_gamemode_peers[i].data[1], _actual_gamemode_peers[i].data[2], _actual_gamemode_peers[i].data[3], _actual_gamemode_peers[i].data[4], _actual_gamemode_peers[i].data[5]);
							}
							#endif
						}
					}

					// Move RX Buffer
					memmove(rx, rx + sizeof(SceNetAdhocctlConnectPacketS2C), sizeof(rx) - sizeof(SceNetAdhocctlConnectPacketS2C));

					// Fix RX Buffer Length
					rxpos -= sizeof(SceNetAdhocctlConnectPacketS2C);
				}
			}
			
			// Disconnect Packet
			else if(rx[0] == OPCODE_DISCONNECT)
			{
				// Enough Data available
				if(rxpos >= sizeof(SceNetAdhocctlDisconnectPacketS2C))
				{
					// Log Incoming Peer Delete Request
					#ifdef DEBUG
					printk("Incoming Peer Data Delete Request...\n");
					#endif
					
					// Cast Packet
					SceNetAdhocctlDisconnectPacketS2C * packet = (SceNetAdhocctlDisconnectPacketS2C *)rx;
					
					// Delete User by IP
					_deleteFriendByIP(packet->ip);
					
					// Update HUD User Count
					#ifdef LOCALHOST_AS_PEER
					setUserCount(_getActivePeerCount());
					#else
					setUserCount(_getActivePeerCount()+1);
					#endif
					
					// Move RX Buffer
					memmove(rx, rx + sizeof(SceNetAdhocctlDisconnectPacketS2C), sizeof(rx) - sizeof(SceNetAdhocctlDisconnectPacketS2C));
					
					// Fix RX Buffer Length
					rxpos -= sizeof(SceNetAdhocctlDisconnectPacketS2C);
				}
			}
			
			// Scan Packet
			else if(rx[0] == OPCODE_SCAN)
			{
				// Enough Data available
				if(rxpos >= sizeof(SceNetAdhocctlScanPacketS2C))
				{
					// Log Incoming Network Information
					
					// Cast Packet
					SceNetAdhocctlScanPacketS2C * packet = (SceNetAdhocctlScanPacketS2C *)rx;

					// Allocate Structure Data
					SceNetAdhocctlScanInfo *group = (SceNetAdhocctlScanInfo *)malloc(sizeof(SceNetAdhocctlScanInfo));

					create_adhocctl_name_buf(group_name_buf, packet->group.data)
					printk("%s: incoming Group Information from group %s\n", __func__, group_name_buf);

					// Allocated Structure Data
					if(group != NULL)
					{
						// Clear Memory
						memset(group, 0, sizeof(SceNetAdhocctlScanInfo));

						_acquireGroupLock();

						// Link to existing Groups
						group->next = _newnetworks;
						
						// Copy Group Name
						group->group_name = packet->group;
						
						// Set Group Host
						group->bssid.mac_addr = packet->mac;
						
						// Link into Group List
						_newnetworks = group;

						_freeGroupLock();
					}
					
					// Move RX Buffer
					memmove(rx, rx + sizeof(SceNetAdhocctlScanPacketS2C), sizeof(rx) - sizeof(SceNetAdhocctlScanPacketS2C));
					
					// Fix RX Buffer Length
					rxpos -= sizeof(SceNetAdhocctlScanPacketS2C);
				}
			}
			
			// Scan Complete Packet
			else if(rx[0] == OPCODE_SCAN_COMPLETE)
			{
				// Log Scan Completion
				#ifdef DEBUG
				printk("Incoming Scan complete response...\n");
				#endif

				_acquireGroupLock();

				_freeNetworkRecursive(_networks);
				_networks = _newnetworks;
				_newnetworks = NULL;

				_freeGroupLock();

				// Change State
				_thread_status = ADHOCCTL_STATE_DISCONNECTED;

				// Notify Event Handlers
				_notifyAdhocctlhandlers(ADHOCCTL_EVENT_SCAN, 0);
				
				// Move RX Buffer
				memmove(rx, rx + 1, sizeof(rx) - 1);
				
				// Fix RX Buffer Length
				rxpos -= 1;
			}
		}
		
		// Reception Update required
		if((sceKernelGetSystemTimeWide() - lastreceptionupdate) > 5000000)
		{
			// Clone Time into Last Update Field
			lastreceptionupdate = sceKernelGetSystemTimeWide();
			
			// Update Reception Level
			union SceNetApctlInfo info;
			if(sceNetApctlGetInfo(PSP_NET_APCTL_INFO_STRENGTH, &info) >= 0) setReceptionPercentage((int)info.strength);
			else setReceptionPercentage(0);
		}
		
		//	Delay Thread (10ms)
		sceKernelDelayThread(10000);
	}
	
	// Set WLAN HUD Reception to 0% on Shutdown
	setReceptionPercentage(0);
	
	// Notify Caller of Shutdown
	_init = -1;
	
	// Log Shutdown
	printk("End of Friend Finder Thread\n");
	
	// Reset Thread Status
	_thread_status = ADHOCCTL_STATE_DISCONNECTED;
	
	// Kill Thread
	sceKernelExitDeleteThread(0);
	
	// Return Success
	return 0;
}

/**
 * Lock Peerlist
 */
void _acquirePeerLock(void)
{
	//#ifdef ENABLE_PEERLOCK
	#if 0
	// Wait for Unlock
	while(_peerlock)
	{
		// Delay Thread
		sceKernelDelayThread(1);
	}
	
	// Lock Access
	_peerlock = 1;
	#endif

	sceKernelLockLwMutex(&peer_lock, 1, 0);
}

/**
 * Unlock Peerlist
 */
void _freePeerLock(void)
{
	//#ifdef ENABLE_PEERLOCK
	#if 0
	// Unlock Access
	_peerlock = 0;
	#endif

	sceKernelUnlockLwMutex(&peer_lock, 1);
}

/**
 * Lock Grouplist
 */
void _acquireGroupLock(void)
{
	sceKernelLockLwMutex(&group_list_lock, 1, 0);
}

/**
 * Unlock Grouplist
 */
void _freeGroupLock(void)
{
	sceKernelUnlockLwMutex(&group_list_lock, 1);
}

/**
 * Lock Network
 */
void _acquireNetworkLock(void)
{
	#ifdef ENABLE_NETLOCK
	// Wait for Unlock
	while(_networklock)
	{
		// Delay Thread
		sceKernelDelayThread(1);
	}
	
	// Lock Access
	_networklock = 1;
	#endif
}

/**
 * Unlock Network
 */
void _freeNetworkLock(void)
{
	#ifdef ENABLE_NETLOCK
	// Unlock Access
	_networklock = 0;
	#endif
}

// hold peer lock before using this
SceNetAdhocctlPeerInfoEmu *_findFriend(void *addr)
{
	SceNetAdhocctlPeerInfoEmu *peer = _friends;
	while (peer != NULL)
	{
		if (_isMacMatch(addr, &peer->mac_addr))
		{
			return peer;
		}
		peer = peer->next;
	}
	return NULL;
}

/**
 * Add Friend to Local List
 * @param packet Friend Information
 */
void _addFriend(SceNetAdhocctlConnectPacketS2C * packet)
{
	_acquirePeerLock();
	SceNetAdhocctlPeerInfoEmu *peer = _findFriend(&packet->mac);
	if (peer != NULL)
	{
		printk("%s: warning, refreshing existing peer %x:%x:%x:%x:%x:%x\n", __func__, packet->mac.data[0], packet->mac.data[1], packet->mac.data[2], packet->mac.data[3], packet->mac.data[4], packet->mac.data[5]);
		peer->nickname = packet->name;
		peer->mac_addr = packet->mac;
		peer->ip_addr = packet->ip;
		// TODO? we are never really using the last_recv field to timeout.... we just assume that the server on tcp don't miss
	}
	_freePeerLock();
	if (peer != NULL)
	{
		return;
	}

	// Allocate Structure
	peer = (SceNetAdhocctlPeerInfoEmu *)malloc(sizeof(SceNetAdhocctlPeerInfo));
	
	// Allocated Structure
	if(peer != NULL)
	{
		// Clear Memory
		memset(peer, 0, sizeof(SceNetAdhocctlPeerInfo));
		
		// Link to existing Peers
		peer->next = _friends;
		
		// Save Nickname
		peer->nickname = packet->name;
		
		// Save MAC Address
		peer->mac_addr = packet->mac;
		
		// Save IP Address
		peer->ip_addr = packet->ip;
		
		// Multithreading Lock
		_acquirePeerLock();
		
		// Link into Peerlist
		_friends = peer;
		
		// Multithreading Unlock
		_freePeerLock();
	}
}

/**
 * Delete Friend from Local List
 * @param ip Friend IP
 */
void _deleteFriendByIP(uint32_t ip)
{
	// Previous Peer Reference
	SceNetAdhocctlPeerInfoEmu * prev = NULL;
	
	// Peer Pointer
	SceNetAdhocctlPeerInfoEmu * peer = _friends;
	
	// Iterate Peers
	for(; peer != NULL; peer = peer->next)
	{
		// Found Peer
		if(peer->ip_addr == ip)
		{
			// Multithreading Lock
			_acquirePeerLock();
			
			// Unlink Left (Beginning)
			if(prev == NULL) _friends = peer->next;
			
			// Unlink Left (Other)
			else prev->next = peer->next;
			
			// Multithreading Unlock
			_freePeerLock();
			
			// Free Memory
			free(peer);
			
			// Stop Search
			break;
		}
		
		// Set Previous Reference
		prev = peer;
	}
}
