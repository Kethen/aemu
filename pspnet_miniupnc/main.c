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

#include <pspsdk.h>
#include <pspkernel.h>
#include "library/common.h"

#include "library/miniwget.h"
#include "library/miniupnpc.h"
#include "library/upnpcommands.h"
#include "library/upnperrors.h"

#define MODULENAME "sceNetMiniUPnP"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER, 1, 6);
PSP_HEAP_SIZE_KB(100);

// Library Status
int _status = 0;

// Resolver ID
int _rid = -1;

// Resolver Buffer
char * rbuf[512];

// UPNP Device List
struct UPNPDev * _devlist = NULL;

// UPNP URL List Anchor
struct UPNPUrls _urls;

// IGD Data Storage
struct IGDdatas _data;

// Local IP Address
union SceNetApctlInfo _info;

// Prototypes
int _mainThread(SceSize args, void * argp);
void SetRedirectAndTest(struct UPNPUrls * urls, struct IGDdatas * data, const char * iaddr, const char * iport, const char * eport, const char * proto, const char * leaseDuration, const char * description);
void RemoveRedirect(struct UPNPUrls * urls, struct IGDdatas * data, const char * eport, const char * proto);

void miniupnc_start()
{
	printk("%s: begin\n", __func__);

	// Create Main Thread
	int update = sceKernelCreateThread("upnp_thread", _mainThread, 0x40, 16384, 0, NULL);

	// Start Main Thread
	if(update >= 0)
	{
		sceKernelStartThread(update, 0, NULL);
		printk("%s: main thread started\n", __func__);
	}
	else
	{
		printk("%s: failed creating upnp thread, 0x%x\n", __func__, update);
	}
}

int rehook_inet();

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");
	rehook_inet();

	return 0;
}

void miniupnc_stop()
{
	printk("%s: begin\n", __func__);

	// Library is running
	if(_status == 1)
	{
		printk("%s: stopping current miniupnc instance\n", __func__);

		// Trigger Library Shutdown
		_status = -1;
		
		// Wait for Shutdown
		while(_status != 0) sceKernelDelayThread(1000);

		printk("%s: current miniupnc instance stopped\n", __func__);
	}
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	printk(MODULENAME " stop!\n");
	
	return 0;
}

// Main Thread
int _mainThread(SceSize args, void * argp)
{
	while(_status != -1){
		#define DELAY_CONTINUE() { \
			sceKernelDelayThread(1000000); \
			continue; \
		}

		int resolver_init_status = sceNetResolverInit();
		if(resolver_init_status != 0)
		{
			printk("%s: failed initializing dns resolver, 0x%x, retrying\n", __func__, resolver_init_status);
			DELAY_CONTINUE();
		}

		int resolver_create_status = sceNetResolverCreate(&_rid, rbuf, sizeof(rbuf));
		if (resolver_create_status != 0)
		{
			sceNetResolverTerm();
			printk("%s: failed creating dns resolver, 0x%x, retrying\n", __func__, resolver_create_status);
			DELAY_CONTINUE();
		}

		int error = 0;
		_devlist = upnpDiscover(2000, NULL, NULL, 0, 0, &error);
		if (_devlist == NULL)
		{
			sceNetResolverDelete(_rid);
			_rid = -1;
			sceNetResolverTerm();
			printk("%s: failed discovering upnp device, %d, retrying\n", __func__, error);
			DELAY_CONTINUE();
		}

		char lanaddr[64];
		int get_igd_status = UPNP_GetValidIGD(_devlist, &_urls, &_data, lanaddr, sizeof(lanaddr));
		if (get_igd_status != 1)
		{
			freeUPNPDevlist(_devlist);
			sceNetResolverDelete(_rid);
			_rid = -1;
			sceNetResolverTerm();
			printk("%s: failed getting igd, %d, retrying\n", __func__, get_igd_status);
			DELAY_CONTINUE();
		}

		int get_local_ip_status = sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &_info);
		if (get_local_ip_status != 0)
		{
			FreeUPNPUrls(&_urls);
			freeUPNPDevlist(_devlist);
			sceNetResolverDelete(_rid);
			_rid = -1;
			sceNetResolverTerm();
			printk("%s: failed getting local ip, 0x%x, retrying\n", __func__, get_local_ip_status);
			DELAY_CONTINUE();
		}

		// Set Library Status
		_status = 1;
		
		// Main Loop
		while(_status == 1)
		{
			// Delay Thread
			sceKernelDelayThread(1000000);
		}

		FreeUPNPUrls(&_urls);
		freeUPNPDevlist(_devlist);
		sceNetResolverDelete(_rid);
		_rid = -1;
		sceNetResolverTerm();
	}

	#if 0	
	// Initialization Error
	if(_status != -1)
	{
		// Unload Module because Router doesn't support UPNP
		int status = 0; sceKernelStopUnloadSelfModule(0, NULL, &status, NULL);
	}
	#endif

	// Reset Library Status
	_status = 0;

	// Kill Thread
	sceKernelExitDeleteThread(0);
	
	// Return Success
	return 0;
}

// Fixes Protocol Prototype Strings
const char * protofix(const char * proto)
{
	// Available Protocol Prototypes
	static const char proto_tcp[4] = { 'T', 'C', 'P', 0};
	static const char proto_udp[4] = { 'U', 'D', 'P', 0};
	
	// Algorithm Variables
	int i, b;
	
	// TCP Comparison
	for(i=0, b=1; i<4; i++) b = b && ((proto[i] == proto_tcp[i]) || (proto[i] == (proto_tcp[i] | 32)));

	// TCP Match
	if(b) return proto_tcp;
	
	// UDP Comparison
	for(i=0, b=1; i<4; i++) b = b && ((proto[i] == proto_udp[i]) || (proto[i] == (proto_udp[i] | 32)));

	// UDP Match
	if(b) return proto_udp;
	
	// Unknown Protocol
	return NULL;
}

// Setup Port Redirect on Router (from libminiupnpc)
void SetRedirectAndTest(struct UPNPUrls * urls, struct IGDdatas * data, const char * iaddr, const char * iport, const char * eport, const char * proto, const char * leaseDuration, const char * description)
{
	char externalIPAddress[40];
	char intClient[40];
	char intPort[6];
	char duration[16];
	int r;

	if(!iaddr || !iport || !eport || !proto)
	{
		return;
	}
	proto = protofix(proto);
	if(!proto)
	{
		return;
	}
	
	UPNP_GetExternalIPAddress(urls->controlURL,
	                          data->first.servicetype,
							  externalIPAddress);
	if(externalIPAddress[0])
		printk("ExternalIPAddress = %s\n", externalIPAddress);
	else
		printk("GetExternalIPAddress failed.\n");
	
	r = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
	                        eport, iport, iaddr, description, proto, 0, leaseDuration);
	if(r!=UPNPCOMMAND_SUCCESS)
		printk("AddPortMapping(%s, %s, %s) failed with code %d (%s)\n",
		       eport, iport, iaddr, r, strupnperror(r));

	r = UPNP_GetSpecificPortMappingEntry(urls->controlURL,
	                                 data->first.servicetype,
    	                             eport, proto,
									 intClient, intPort, NULL/*desc*/,
	                                 NULL/*enabled*/, duration);
	if(r!=UPNPCOMMAND_SUCCESS)
		printk("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
		       r, strupnperror(r));
	
	if(intClient[0]) {
		printk("InternalIP:Port = %s:%s\n", intClient, intPort);
		printk("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
		       externalIPAddress, eport, proto, intClient, intPort, duration);
	}
}

// Delete Port Redirect on Router (from libminiupnpc)
void RemoveRedirect(struct UPNPUrls * urls, struct IGDdatas * data, const char * eport, const char * proto)
{
	int r;
	if(!proto || !eport)
	{
		return;
	}
	proto = protofix(proto);
	if(!proto)
	{
		return;
	}
	r = UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, eport, proto, 0);
}

// Create Port Forward
void sceNetPortOpen(const char * protocol, uint16_t port)
{
	printk("%s: opening %s %u\n", __func__, protocol, port);

	// Wait for Initialization
	int wait_seconds = 5;
	while(wait_seconds > 0 && _status != 1)
	{
		sceKernelDelayThread(1000000);
		wait_seconds--;
	}
	
	if (_status != 1)
	{
		printk("%s: cannot create port forward yet, the library is not ready\n", __func__);
		return;
	}
	
	// Argument Check
	if(protofix(protocol) != NULL && port != 0)
	{
		// Port Buffer
		char sport[6]; sprintf(sport, "%u", port);
		
		// Forward Port to Localhost
		SetRedirectAndTest(&_urls, &_data, _info.ip, sport, sport, protocol, "0", "PRO Online Game Client");
	}
}

// Delete Port Forward
void sceNetPortClose(const char * protocol, uint16_t port)
{
	printk("%s: closing %s %u\n", __func__, protocol, port);

	// Wait for Initialization
	int wait_seconds = 5;
	while(wait_seconds > 0 && _status != 1)
	{
		sceKernelDelayThread(1000000);
		wait_seconds--;
	}
	
	if (_status != 1)
	{
		printk("%s: cannot close port forward yet, the library is not ready\n", __func__);
		return;
	}
	
	// Argument Check
	if(protofix(protocol) != NULL && port != 0)
	{
		// Port Buffer
		char sport[6]; sprintf(sport, "%u", port);
		
		// Remove Port Forward
		RemoveRedirect(&_urls, &_data, sport, protocol);
	}
}

// XXX hack https://github.com/pspdev/psplinkusb/pull/52
void _exit(int status)
{
	sceKernelExitGame();
}
