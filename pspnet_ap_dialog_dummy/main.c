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
#include <psputility.h>
#include <pspnet_apctl.h>

#include <string.h>

#define MODULENAME "sceNetApDialogDummy_Library"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_USER + 6, 1, 6);
PSP_HEAP_SIZE_KB(HEAP_SIZE);

int printk(...);

int sceNetApDialogDummyInit(){
	printk("%s: begin\n", __func__);
	// can it return anything else?
	return 0;
}

int sceNetApDialogDummyConnect(){
	// adapt to modern firmware utility, based on https://github.com/pspdev/pspsdk/blob/9f6f4c0c10921e15339b0960b5069ff8a8f9fcd8/src/samples/utility/netdialog/main.c#L191
	pspUtilityNetconfData data;

	int lang = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &lang);

	int ctrl = PSP_UTILITY_ACCEPT_CROSS;
	sceUtilityGetSystemParamInt(9, &ctrl);

	memset(&data, 0, sizeof(data));
	data.base.size = sizeof(data);
	data.base.language = lang;
	data.base.buttonSwap = ctrl;
	data.base.graphicsThread = 17;
	data.base.accessThread = 19;
	data.base.fontThread = 18;
	data.base.soundThread = 16;
	data.action = PSP_NETCONF_ACTION_CONNECTAP;
	// We don't have enough ram for a browser, unless we pre allocate more, but that might hurt game compat
	data.hotspot = 0;
	
	struct pspUtilityNetconfAdhoc adhocparam;
	memset(&adhocparam, 0, sizeof(adhocparam));

	int result = sceUtilityNetconfInitStart(&data);
	printk("%s: returning 0x%x/%d\n", __func__, result, result);
	return result;
}

int sceNetApDialogDummyGetState(){
	// Finger cross that the ge state is good enough for utility drawing

	// No way to reliably tell how fast the animation should be, set to 1 frame
	sceUtilityNetconfUpdate(1);

	// Get state
	int state = sceUtilityNetconfGetStatus();

	// Translate state 
	switch(state){
		case PSP_UTILITY_DIALOG_NONE:
			state = 0;
			break;
		case PSP_UTILITY_DIALOG_INIT:
		case PSP_UTILITY_DIALOG_VISIBLE:
			state = 1;
			break;
		case PSP_UTILITY_DIALOG_FINISHED:
		case PSP_UTILITY_DIALOG_QUIT:
			int connection_state = 0;
			int get_state_status = sceNetApctlGetState(&connection_state);
			if (get_state_status != 0){
				state = 3;
				break;
			}
			if (connection_state == PSP_NET_APCTL_STATE_GOT_IP){
				state = 2;
				break;
			}else{
				state = 3;
				break;
			}
	}

	return state;
}

int sceNetApDialogDummyTerm(){
	printk("%s: begin\n", __func__);
	return sceUtilityNetconfShutdownStart();
}

void init_littlec();
void clean_littlec();

// Module Start Event
int module_start(SceSize args, void * argp)
{
	printk(MODULENAME " start!\n");
	init_littlec();

	return 0;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	printk(MODULENAME " stop!\n");
	clean_littlec();

	return 0;
}
