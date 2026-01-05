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

#include "../../common.h"

static uint32_t server_ip = 0;

uint32_t resolve_server_ip(){
	sceKernelWaitSema(_server_resolve_mutex, 1, 0);

	if (server_ip != 0){
		sceKernelSignalSema(_server_resolve_mutex, 1);
		return server_ip;
	}

	int dns_init_status = sceNetResolverInit();
	if (dns_init_status != 0){
		sceKernelSignalSema(_server_resolve_mutex, 1);
		printk("%s: failed initializing dns resolver\n", __func__, dns_init_status);
		return 0xFFFFFFFF;
	}

	unsigned char rbuf[512]; int rid = 0;
	int dns_create_status = sceNetResolverCreate(&rid, rbuf, sizeof(rbuf));
	if (dns_create_status != 0){
		sceNetResolverTerm();
		sceKernelSignalSema(_server_resolve_mutex, 1);
		printk("%s: failed creating dns resolver instance\n", __func__);
		return 0xFFFFFFFF;
	}

	int fd = sceIoOpen("ms0:/seplugins/server.txt", PSP_O_RDONLY, 0777);
	if (fd < 0){
		fd = sceIoOpen("ms0:/PSP/PLUGINS/atpro/server.txt", PSP_O_RDONLY, 0777);
	}
	if (fd < 0){
		sceNetResolverDelete(rid);
		sceNetResolverTerm();
		sceKernelSignalSema(_server_resolve_mutex, 1);
		printk("%s: failed opening server.txt for reading, 0x%x\n", __func__, fd);
		return 0xFFFFFFFF;
	}
	char namebuf[128] = {0};
	int read_status = sceIoRead(fd, namebuf, 127);
	sceIoClose(fd);
	if (read_status < 0){
		sceNetResolverDelete(rid);
		sceNetResolverTerm();
		sceKernelSignalSema(_server_resolve_mutex, 1);
		printk("%s: failed reading server.txt, 0x%x\n", __func__, read_status);
		return 0xFFFFFFFF;
	}
	for(int i = 0;i < 127;i++){
		if (namebuf[i] == '\0'){
			break;
		}
		if (namebuf[i] == '\r' || namebuf[i] == '\n'){
			namebuf[i] = '\0';
		}
	}

	uint32_t pending_ip = 0;
	int resolve_status = sceNetResolverStartNtoA(rid, namebuf, (struct in_addr *)&pending_ip, 500000, 2);
	sceNetResolverDelete(rid);
	sceNetResolverTerm();
	if (resolve_status){
		printk("%s: failed resolving %s as donamin name, trying as ip address\n", __func__, namebuf);
		int aton_status = sceNetInetInetAton(namebuf, &pending_ip);
		if (aton_status == 0){
			printk("%s: %s is not a valid ip address either\n", __func__, namebuf);
			pending_ip = 0xFFFFFFFF;
		}
	}

	if (pending_ip != 0xFFFFFFFF){
		server_ip = pending_ip;
	}
	sceKernelSignalSema(_server_resolve_mutex, 1);

	if (pending_ip == 0xFFFFFFFF){
		printk("%s: dns resolve failed\n", __func__);
		return 0xFFFFFFFF;
	}

	printk("%s: server %s resolved as 0x%x\n", __func__, namebuf, pending_ip);

	return server_ip;
}

uint16_t htons(uint16_t host){
	uint16_t ret;
	char *host_bytes = (char *)&host;
	char *ret_bytes = (char *)&ret;
	ret_bytes[0] = host_bytes[1];
	ret_bytes[1] = host_bytes[0];
	return ret;
}

static int pdp_create_postoffice(const SceNetEtherAddr *saddr, int sport, int bufsize){
	struct aemu_post_office_sock_addr addr = {
		.addr = resolve_server_ip(),
		.port = htons(POSTOFFICE_PORT)
	};

	int state = 0;
	void *pdp_sock = pdp_create_v4(&addr, (const char *)saddr, sport, &state);
	if (pdp_sock == NULL){
		printk("%s: failed creating post office pdp socket, %d\n", __func__, state);
		return NET_NO_SPACE;
	}

	AdhocSocket *internal = (AdhocSocket *)malloc(sizeof(AdhocSocket));
	if (internal == NULL){
		printk("%s: failed allocating space for pdp socket entry\n", __func__);
		pdp_delete(pdp_sock);
		return NET_NO_SPACE;
	}

	internal->is_ptp = false;
	internal->postoffice_handle = pdp_sock;
	internal->pdp.laddr = *saddr;
	internal->pdp.lport = sport;
	internal->pdp.rcv_sb_cc = bufsize;

	sceKernelWaitSema(_socket_mapper_mutex, 1, 0);

	AdhocSocket **free_slot = NULL;
	int i;
	for(i = 0;i < 255; i++){
		if(_sockets[i] == NULL){
			free_slot = &_sockets[i];
			break;
		}
	}
	if (free_slot == NULL){
		sceKernelSignalSema(_socket_mapper_mutex, 1);
		printk("%s: failed finding a free slot to keep track of the socket\n", __func__);
		free(internal);
		pdp_delete(pdp_sock);
		return NET_NO_SPACE;
	}
	*free_slot = internal;
	sceKernelSignalSema(_socket_mapper_mutex, 1);
	return i + 1;
}

/**
 * Adhoc Emulator PDP Socket Creator
 * @param saddr Local MAC (Unused)
 * @param sport Local Binding Port
 * @param bufsize Socket Buffer Size
 * @param flag Bitflags (Unused)
 * @return Socket ID > 0 on success or... ADHOC_NOT_INITIALIZED, ADHOC_INVALID_ARG, ADHOC_SOCKET_ID_NOT_AVAIL, ADHOC_INVALID_ADDR, ADHOC_PORT_NOT_AVAIL, ADHOC_INVALID_PORT, ADHOC_PORT_IN_USE, NET_NO_SPACE
 */
int proNetAdhocPdpCreate(const SceNetEtherAddr * saddr, uint16_t sport, int bufsize, int flag)
{
	// Library is initialized
	if(_init)
	{
		// Valid Arguments are supplied
		if(saddr != NULL && bufsize > 0)
		{
			// Just force local mac for now, seems fine on PPSSPP
			SceNetEtherAddr local_mac = {0};
			sceNetGetLocalEtherAddr(&local_mac);
			#ifdef DEBUG
			if (!_isMacMatch(&local_mac, saddr))
			{
				printk("%s: createing pdp with a non local mac..?\n", __func__);
				printk("local %x:%x:%x:%x:%x:%x\n", local_mac.data[0], local_mac.data[1], local_mac.data[2], local_mac.data[3], local_mac.data[4], local_mac.data[5]);
				printk("desired %x:%x:%x:%x:%x:%x\n", saddr->data[0], saddr->data[1], saddr->data[2], saddr->data[3], saddr->data[4], saddr->data[5]);
			}
			#endif
			// Valid MAC supplied
			//if(_IsLocalMAC(saddr))
			{
				// Random Port required
				if(sport == 0)
				{
					// Find unused Port
					while(sport < 0 || _IsPDPPortInUse(sport))
					{
						// Generate Port Number
						sport = (uint16_t)_getRandomNumber(65535);
					}
				}else
				{
					// Warn about priv ports for playing with PPSSPP
					if (__builtin_expect(sport < 1024, 0))
						printk("%s: using sport %d, might have issues when playing with PPSSPP, change your port offset here and on PPSSPP\n", __func__, sport);
				}
				
				// Unused port supplied
				if(!_IsPDPPortInUse(sport))
				{
					if (_postoffice){
						return pdp_create_postoffice(&local_mac, sport, bufsize);
					}

					// Create Internet UDP Socket
					int socket = sceNetInetSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
					
					// Valid Socket produced
					if(socket > 0)
					{
						// Enable Port Re-use
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &_one, sizeof(_one));
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &_one, sizeof(_one));

						#if 0
						// set limits like ppsspp does
						int opt = bufsize * 5;
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
						opt = bufsize * 10;
						sceNetInetSetsockopt(socket, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
						#endif

						// Binding Information for local Port
						SceNetInetSockaddrIn addr;
						addr.sin_len = sizeof(addr);
						addr.sin_family = AF_INET;
						addr.sin_addr = INADDR_ANY;
						addr.sin_port = sceNetHtons(sport);
						
						// Bound Socket to local Port
						if(sceNetInetBind(socket, (SceNetInetSockaddr *)&addr, sizeof(addr)) == 0)
						{
							// Allocate Memory for Internal Data
							AdhocSocket* internal = (AdhocSocket *)malloc(sizeof(AdhocSocket));
							
							// Allocated Memory
							if(internal != NULL)
							{
								// Clear Memory
								memset(internal, 0, sizeof(AdhocSocket));
								
								// Find Free Translator Index
								sceKernelWaitSema(_socket_mapper_mutex, 1, 0);
								int i = 0; for(; i < 255; i++) if(_sockets[i] == NULL) break;
								
								// Found Free Translator Index
								if(i < 255)
								{
									// Fill in Data
									internal->pdp.id = socket;
									//internal->pdp.laddr = *saddr;
									internal->pdp.laddr = local_mac;
									internal->pdp.lport = sport;
									internal->pdp.rcv_sb_cc = bufsize;
									
									// Link Socket to Translator ID
									_sockets[i] = internal;
									
									// Forward Port on Router
									sceNetPortOpen("UDP", sport);

									sceKernelSignalSema(_socket_mapper_mutex, 1);
									
									// Success
									return i + 1;
								}
								sceKernelSignalSema(_socket_mapper_mutex, 1);
								
								// Free Memory for Internal Data
								free(internal);
							}
						}
						
						// Close Socket
						sceNetInetClose(socket);
					}
					
					// Default to No-Space Error
					return NET_NO_SPACE;
				}
				
				// Port is in use by another PDP Socket
				return ADHOC_PORT_IN_USE;
			}
			
			// Invalid MAC supplied
			return ADHOC_INVALID_ADDR;
		}
		
		// Invalid Arguments were supplied
		return ADHOC_INVALID_ARG;
	}
	
	// Library is uninitialized
	return ADHOC_NOT_INITIALIZED;
}

/**
 * Local MAC Check
 * @param saddr To-be-checked MAC Address
 * @return 1 if valid or... 0
 */
int _IsLocalMAC(const SceNetEtherAddr * addr)
{
	// Get Local MAC Address
	SceNetEtherAddr saddr;
	sceNetGetLocalEtherAddr(&saddr);
	
	// Compare MAC Addresses
	int match = memcmp((const void *)addr, (const void *)&saddr, ETHER_ADDR_LEN);
	
	// Return Result
	return (match == 0);
}

/**
 * PDP Port Check
 * @param port To-be-checked Port
 * @return 1 if in use or... 0
 */
int _IsPDPPortInUse(uint16_t port)
{
	// Iterate Elements
	int i = 0; for(; i < 255; i++) if(_sockets[i] != NULL && !_sockets[i]->is_ptp && _sockets[i]->pdp.lport == port) return 1;
	
	// Unused Port
	return 0;
}
