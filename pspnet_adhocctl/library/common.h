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

#ifndef _ADHOCCTL_COMMON_H_
#define _ADHOCCTL_COMMON_H_

// --- Basic Includes ---

// Basic Types
#include <stdint.h>

// Heap Control
#include <malloc.h>

// String Handling
#include <string.h>

// Time Access
#include <time.h>

// wait pspwlan.h did not carry u8 type itself huh
typedef uint8_t u8;
// WLAN Switch Access
#include <pspwlan.h>

// Access Point Control
#include <pspnet_apctl.h>

// Netconfig Utility
#include <psputility.h>

// PSP thread control
#include <pspthreadman.h>

// PSP Module Loader
#include <pspmodulemgr.h>

// PSP file IO
#include <pspiofilemgr.h>

// Drawing new dialog
#include <pspdisplay.h>
#include <pspge.h>

// --- Kernel Debugger ---

// Kernel Debug Printer
#include "../../atpro/logs.h"

// --- Emulator Basics ---

// Adhocctl Library Types
#include "structures.h"

// Adhocctl Global Data
#include "globals.h"

// Adhocctl Common Functions
#include "functions.h"

// --- Improved PSPSDK Header ---

// Internet Library
#include "../../pspnet_adhoc/library/pspsdk/pspnet_inet.h"

// Networking Library
#include "../../pspnet_adhoc/library/pspsdk/pspnet.h"

// DNS Resolver
#include "pspsdk/pspnet_resolver.h"

// --- Global Library Calls ---

// sceNetAdhocctlInit
#include "calls/init.h"

// sceNetAdhocctlTerm
#include "calls/term.h"

// sceNetAdhocctlGetState
#include "calls/getstate.h"

// sceNetAdhocctlGetAdhocId
#include "calls/getadhocid.h"

// sceNetAdhocctlGetParameter
#include "calls/getparameter.h"

// sceNetAdhocctlAddHandler
#include "calls/addhandler.h"

// sceNetAdhocctlDelHandler
#include "calls/delhandler.h"

// sceNetAdhocctlScan
#include "calls/scan.h"

// sceNetAdhocctlGetScanInfo
#include "calls/getscaninfo.h"

// sceNetAdhocctlGetPeerInfo
#include "calls/getpeerinfo.h"

// sceNetAdhocctlGetPeerList
#include "calls/getpeerlist.h"

// sceNetAdhocctlGetAddrByName
#include "calls/getaddrbyname.h"

// sceNetAdhocctlGetNameByAddr
#include "calls/getnamebyaddr.h"

// --- Adhoc Mode Calls ---

// sceNetAdhocctlCreate
#include "calls/adhocmode/create.h"

// sceNetAdhocctlConnect
#include "calls/adhocmode/connect.h"

// sceNetAdhocctlJoin
#include "calls/adhocmode/join.h"

// sceNetAdhocctlDisconnect
#include "calls/adhocmode/disconnect.h"

// --- Game Mode Calls ---

// sceNetAdhocctlCreateEnterGameMode
#include "calls/gamemode/create.h"

// sceNetAdhocctlCreateEnterGameModeMin
#include "calls/gamemode/createmin.h"

// sceNetAdhocctlJoinEnterGameMode
#include "calls/gamemode/join.h"

// sceNetAdhocctlExitGameMode
#include "calls/gamemode/exit.h"

// sceNetAdhocctlGetGameModeInfo
#include "calls/gamemode/getinfo.h"

// --- Kernel Utility Calls ---

// sceUtilityNetconfInitStart
#include "calls/netconf/init.h"

// sceUtilityNetconfGetStatus
#include "calls/netconf/getstatus.h"

// sceUtilityNetconfUpdate
#include "calls/netconf/update.h"

// sceUtilityNetconfShutdownStart
#include "calls/netconf/shutdown.h"

#endif
