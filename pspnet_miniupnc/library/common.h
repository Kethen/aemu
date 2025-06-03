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

#ifndef _MINIUPNP_COMMON_H_
#define _MINIUPNP_COMMON_H_

// --- Basic Includes ---

// Input & Output
#include <stdio.h>

// Basic Types
#include <stdint.h>

// Basic type conversion
#include <ctype.h>

// Standard Library
#include <stdlib.h>

// Heap Control
#include <malloc.h>

// String Handling
#include <string.h>

// Time Access
#include <time.h>

typedef uint8_t u8;
// WLAN Switch Access
#include <pspwlan.h>

// Access Point Control
#include <pspnet_apctl.h>

// Netconfig Utility
#include <psputility.h>

// --- Kernel Debugger ---

// Kernel Debug Printer
#include "../../atpro/logs.h"

// --- Library Stuff ---

// Structures
#include "structures.h"

// Globals
#include "globals.h"

// --- Improved PSPSDK Header ---

// Internet Library
#include "../../pspnet_adhoc/library/pspsdk/pspnet_inet.h"

// Networking Library
#include "../../pspnet_adhoc/library/pspsdk/pspnet.h"

// DNS Resolver
#include "../../pspnet_adhocctl/library/pspsdk/pspnet_resolver.h"

#endif
