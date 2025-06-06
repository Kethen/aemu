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

#ifndef _ADHOC_MATCHING_COMMON_H_
#define _ADHOC_MATCHING_COMMON_H_

// --- Basic Includes ---

// Basic Types
#include <stdint.h>

// Heap Control
#include <malloc.h>

// Standard IO (for sprintf)
#include <stdio.h>

// String Handling
#include <string.h>

// Time Access
#include <time.h>

// XXX
typedef uint8_t u8;
// WLAN Switch Access
#include <pspwlan.h>

// Netconfig Utility
#include <psputility.h>

// Threading
#include <pspthreadman.h>

// --- Kernel Debugger ---

// Kernel Debug Printer
#include "../../atpro/logs.h"

// --- Emulator Basics ---

// Adhoc Matching Library Types
#include "structures.h"

// Adhoc Matching Global Data
#include "globals.h"

// Adhoc Matching Common Functions
#include "functions.h"

// PSP thread control
#include <pspthreadman.h>

// --- Improved PSPSDK Header ---

// Basic Network Library
#include "../../pspnet_adhoc/library/pspsdk/pspnet.h"

// Adhoc Socket Library
#include "pspsdk/pspnet_adhoc.h"

// --- Global Library Calls ---

// sceNetAdhocMatchingInit
#include "calls/init.h"

// sceNetAdhocMatchingTerm
#include "calls/term.h"

// sceNetAdhocMatchingCreate
#include "calls/create.h"

// sceNetAdhocMatchingStart
#include "calls/start.h"

// sceNetAdhocMatchingStop
#include "calls/stop.h"

// sceNetAdhocMatchingDelete
#include "calls/delete.h"

// sceNetAdhocMatchingSelectTarget
#include "calls/select.h"

// sceNetAdhocMatchingCancelTarget(WithOpt)
#include "calls/cancel.h"

// sceNetAdhocMatchingSendData
#include "calls/send.h"

// sceNetAdhocMatchingAbortSendData
#include "calls/abortsend.h"

// sceNetAdhocMatchingSetHelloOpt
#include "calls/sethello.h"

// sceNetAdhocMatchingGetHelloOpt
#include "calls/gethello.h"

// sceNetAdhocMatchingGetMembers
#include "calls/getmembers.h"

// sceNetAdhocMatchingGetPoolMaxAlloc
#include "calls/getpoolmaxalloc.h"

// sceNetAdhocMatchingGetPoolStat
#include "calls/getpoolstat.h"

#endif

