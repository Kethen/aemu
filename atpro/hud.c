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

#include <string.h>
#include <stdio.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspsysmem_kernel.h>
#include <psppower.h>
#include <psprtc.h>
#include <pspimpose_driver.h>
#include "hud.h"

// Logical Constants
#define NICKNAME_TRUNCATE 8
#define KEYPHRASE_TRUNCATE 8
#define CHAT_TRUNCATE 63
#define CHAT_MAX_LINES 8
#define KEYPHRASE_COUNT (8*4)
#define OUTBOX_SIZE 5

// Graphical Constants
#define HORIZONTAL_PADDING 12
#define VERTICAL_PADDING 10
#define FONT_WIDTH 6
#define BUTTON_CLONE_SHIFT 279
#define BUTTON_MAPPINGS_ON_SCREEN 8
#define BOTTOM_LINE 256
#define EXIT_MESSAGE_X 227
#define MIDDLE_MESSAGE_Y 132
#define MIDDLE_MESSAGE_X 90
#define CHAT_BINDING_X 221
#define INPUT_LINE_Y 140

// Graphical Macros
#define GET_MIDDLE_ALIGN_X(t) (240 - strlen(t) * FONT_WIDTH / 2)
#define GET_MIDDLE_ALIGN_Y(t) (136 - strlen(t) * FONT_WIDTH / 2)

// Position for Button Mapping (8-Character optimized)
// UP, LEFT, DOWN, RIGHT, TRIANGLE, SQUARE, CROSS, CIRCLE
int bpos_x[8] = {  76 + 25,  12 + 25,  76 + 25, 140 + 25 };
int bpos_y[8] = { 158, 195, 234, 195 };

// Chat Message Structure
typedef struct
{
	char sender[NICKNAME_TRUNCATE + 1];
	char message[CHAT_TRUNCATE + 1];
	uint32_t color;
} ChatLog;

// Chat Messages
static ChatLog chatlog[CHAT_MAX_LINES];

// Outbox
static char outbox[OUTBOX_SIZE][CHAT_TRUNCATE + 1];

// Keyphrases
static char * keyphrases[KEYPHRASE_COUNT] = {
	"A", "B", "C", "D", "E", "F", "G", "H",
	"I", "J", "K", "L", "M", "N", "O", "P",
	"Q", "R", "S", "T", "U", "V", "W", "X",
	"Y", "Z", "!", "?", ".", ",", "\x80", "SPACE"
};

// Chat Line Buffer
static char chatline[CHAT_TRUNCATE + 1];

// Phrase Package Index (0 = Standard, 1 = L Button, 2 = R Button, 3 = L+R Button)
int phrasepackage = 0;

// Online Mode Switch
extern int onlinemode;

// Server Connectivity Status
int connected_to_prometheus = 0;

// User Count
int usercount = 1;

// Show Notification Flag
int shownotification = 0;

// Show Server Notification Flag
int showservernotification = 0;

// Last reported WLAN reception level (in percent)
int reception = 0;

// Last reported Battery charge (in percent)
int batterycharge = 0;

// Send Button pressed
int is_send_button_pressed = 0;

// Send Button Clock Variables
SceKernelSysClock send_press_start, send_press_end;

// Delete Button pressed
int is_delete_button_pressed = 0;

// Delete Button Clock Variables
SceKernelSysClock delete_press_start, delete_press_end;

// Last Time Battery got queried
uint64_t lastbatteryquery = 0;

// Paint HUD Overlay
void drawInfo(CANVAS * canvas)
{
	// Online Mode
	if(onlinemode)
	{
		// Erase Notification Flags
		shownotification = 0;
		showservernotification = 0;
		
		// Connected to Server
		if(connected_to_prometheus)
		{
			// Chat Headline
			drawSmallFont(canvas, "PROMETHEUS CHATROOM", HORIZONTAL_PADDING + 48, VERTICAL_PADDING, RGB_8888(0xDE, 0xDE, 0xDE));
			
			// Chat Output
			int i = 0; for(; i < CHAT_MAX_LINES; i++)
			{
				// End of Chat Log
				if(chatlog[i].message[0] == 0) break;
				
				// Print 8-Character Truncated Nickname
				drawSmallFont(canvas, chatlog[i].sender, HORIZONTAL_PADDING, VERTICAL_PADDING + FONT_WIDTH * (i+1) + 5, chatlog[i].color);
				
				// Print Delimiter
				drawSmallFont(canvas, ":", HORIZONTAL_PADDING + FONT_WIDTH * NICKNAME_TRUNCATE, VERTICAL_PADDING + FONT_WIDTH * (i+1) + 5, RGB_8888(0xFF, 0xFF, 0xFF));
				
				// Print 63-Character Text Message
				drawSmallFont(canvas, chatlog[i].message, HORIZONTAL_PADDING + FONT_WIDTH * NICKNAME_TRUNCATE + FONT_WIDTH, VERTICAL_PADDING + FONT_WIDTH * (i+1) + 5, chatlog[i].color);
			}
			
			// Fixed Chat Line Buffer
			char fixedchatline[CHAT_TRUNCATE + 1];
			
			// Fix Chat Line
			int j = 0; for(; j < strlen(chatline); j++)
			{
				// Fix Character
				fixedchatline[j] = convertChar((uint8_t)chatline[j]);
			}
			
			// Terminate Fixed Chat Line Copy
			fixedchatline[j] = 0;
			
			// Draw Chat Line
			drawSmallFont(canvas, fixedchatline, HORIZONTAL_PADDING, INPUT_LINE_Y, RGB_8888(0x00, 0xFF, 0x00));
			
			// Fix Keyphrase Index
			int phraseindex = BUTTON_MAPPINGS_ON_SCREEN * phrasepackage;
			
			// Button Mapping Example
			for(i = 0; i < BUTTON_MAPPINGS_ON_SCREEN; i++)
			{
				// End of Keyphrase List
				if(keyphrases[phraseindex + i][0] == 0) break;
				
				// Location Shift
				int shift = (i > 3) ? (BUTTON_CLONE_SHIFT) : (0);
				
				// Fix Location Index
				int index = (i > 3) ? (i - 4) : (i);
				
				// Truncated Keyphrase Buffer
				char truncphrase[KEYPHRASE_TRUNCATE + 1];
				memset(truncphrase, 0, sizeof(truncphrase));
				strncpy(truncphrase, keyphrases[phraseindex + i], KEYPHRASE_TRUNCATE + 1);
				if(truncphrase[KEYPHRASE_TRUNCATE] != 0)
				{
					truncphrase[KEYPHRASE_TRUNCATE - 1] = '+';
					truncphrase[KEYPHRASE_TRUNCATE] = 0;
				}
				
				// Fix Truncated Keyphrase
				for(j = 0; j < strlen(truncphrase); j++)
				{
					// Fix Character
					truncphrase[j] = convertChar((uint8_t)truncphrase[j]);
				}
				
				// Print 8-Character Truncated Keyphrase
				drawSmallFont(canvas, truncphrase, bpos_x[index] + shift - strlen(truncphrase) * FONT_WIDTH / 2, bpos_y[index], RGB_8888(0xDE, 0xDE, 0xDE));
			}
			
			// User Counter Example
			char strusercount[32]; sprintf(strusercount, "GROUP USER COUNT: %d", usercount);
			drawSmallFont(canvas, strusercount, HORIZONTAL_PADDING, BOTTOM_LINE, RGB_8888(0x00, 0xFF, 0x00));
			
			// Chat Button Binding
			drawSmallFont(canvas, "SELECT: CLEAR CHAT   TAP START: SEND TEXT", CHAT_BINDING_X, VERTICAL_PADDING, RGB_8888(0x00, 0xFF, 0x00));
		}
		
		// Currently Waiting for Network SSID Assignment
		else
		{
			// Write Waiting Headline
			drawSmallFont(canvas, "WAITING FOR ADHOC SSID SETUP...", HORIZONTAL_PADDING + 48, VERTICAL_PADDING, RGB_8888(0xDE, 0xDE, 0xDE));
		}
		
		// Show WLAN Reception
		fillRectangle(canvas, 0, 0, 480, 5, RGB_8888(0xFF, 0xFF, 0xFF), RGB_8888(0x00, 0x00, 0x00));
		fillRectangle(canvas, 0, 0, (int)(4.80f * reception), 5, RGB_8888(0x00, 0x00, 0xFF), RGB_8888(0x00, 0x00, 0x00));
	}
	
	// Offline Mode
	else
	{
		// Offline Mode Headline
		drawSmallFont(canvas, "YOU'RE IN OFFLINE MODE! CHATROOM HAS BEEN DISABLED!", MIDDLE_MESSAGE_X, MIDDLE_MESSAGE_Y, RGB_8888(0xDE, 0xDE, 0xDE));
	}
	
	// Battery Query required
	if((sceKernelGetSystemTimeWide() - lastbatteryquery) > 5000000)
	{
		// Clone Time into Last Query Field
		lastbatteryquery = sceKernelGetSystemTimeWide();
		
		// Update Battery Level
		if(scePowerIsBatteryExist())
			batterycharge = scePowerGetBatteryLifePercent();
		else
			batterycharge = 0;
	}
	
	// Show Battery Status
	if(sceKernelGetModel() != 4)
	{
		fillRectangle(canvas, 0, 267, 480, 5, RGB_8888(0xFF, 0xFF, 0xFF), RGB_8888(0x00, 0x00, 0x00));
		fillRectangle(canvas, 0, 267, (int)(4.80f * batterycharge), 5, RGB_8888(0x00, 0xFF, 0x00), RGB_8888(0x00, 0x00, 0x00));
	}
	
	// Show Speak Volume (0-30)
	int volume = sceImposeGetParam(PSP_IMPOSE_MAIN_VOLUME);
	char * volumestr = "VOLUME";
	drawSmallFont(canvas, volumestr, GET_MIDDLE_ALIGN_X(volumestr), 190, RGB_8888(0xFF, 0xFF, 0xFF));
	fillRectangle(canvas, 211, 205, 60, 5, RGB_8888(0xFF, 0xFF, 0xFF), RGB_8888(0x00, 0x00, 0x00));
	fillRectangle(canvas, 211, 205, 2 * volume, 5, RGB_8888(0xA0, 0xA0, 0xA0), RGB_8888(0x00, 0x00, 0x00));
	
	// Show Local PSP Time
	ScePspDateTime local_time;
	uint32_t k1 = pspSdkSetK1(0);
	sceRtcGetCurrentClockLocalTime(&local_time);
	pspSdkSetK1(k1);
	char time[16]; sprintf(time, "%02u:%02u", local_time.hour, local_time.minute);
	drawSmallFont(canvas, time, HORIZONTAL_PADDING, VERTICAL_PADDING, RGB_8888(0xFF, 0xFF, 0xFF));
	
	// Draw Exit Example
	drawSmallFont(canvas, "HOME: CLOSE MENU   HOLD START: EXIT GAME", EXIT_MESSAGE_X, BOTTOM_LINE, RGB_8888(0x00, 0xFF, 0x00));
}

// Paint Notification Overlay
void drawNotification(CANVAS * canvas)
{
	// New Incoming Server Notice
	if(showservernotification)
	{
		// Notification Text
		char * text = "INCOMING SERVER NOTICE";
		
		// Draw Text Centered
		drawSmallFont(canvas, text, GET_MIDDLE_ALIGN_X(text), BOTTOM_LINE, RGB_8888(0xFF, 0xD7, 0x00));
	}
	
	// New Incoming Message
	else if(shownotification)
	{
		// Notification Text
		char * text = "INCOMING MESSAGE";
		
		// Draw Text Centered
		drawSmallFont(canvas, text, GET_MIDDLE_ALIGN_X(text), BOTTOM_LINE, RGB_8888(0x00, 0xFF, 0x00));
	}
}

// Add Chat Log
void addChatLog(char * nickname, char * text)
{
	// Chat Log Reference
	ChatLog * storage = NULL;
	
	// Find free Chat Log Slot
	int i = 0; for(; i < CHAT_MAX_LINES; i++)
	{
		// Found free Chat Log Slot
		if(chatlog[i].message[0] == 0)
		{
			// Link Field
			storage = &chatlog[i];
			
			// Stop Search
			break;
		}
	}
	
	// Chat Log full
	if(storage == NULL)
	{
		// Move Chat Logs
		for(i = 0; i < CHAT_MAX_LINES - 1; i++)
		{
			// Move Chat Log
			chatlog[i] = chatlog[i+1];
		}
		
		// Clear Last Chat Log
		memset(&chatlog[i], 0, sizeof(chatlog[i]));
		
		// Link Field
		storage = &chatlog[i];
	}
	
	// Store Nickname
	strncpy(storage->sender, nickname, NICKNAME_TRUNCATE);
	
	// Store Message
	strncpy(storage->message, text, CHAT_TRUNCATE);
	
	// Fix Nickname
	for(i = 0; i < strlen(storage->sender); i++)
	{
		// Fix Character
		storage->sender[i] = convertChar((uint8_t)storage->sender[i]);
	}
	
	// Fix Message
	for(i = 0; i < strlen(storage->message); i++)
	{
		// Fix Character
		storage->message[i] = convertChar((uint8_t)storage->message[i]);
	}
	
	// Set Message Color
	storage->color = RGB_8888(0xFF, 0xFF, 0xFF);
	
	// Server Notification
	if(nickname[0] == 0)
	{
		// Fix Sender Name
		strcpy(storage->sender, "SERVER");
		
		// Activate Notification
		showservernotification = 1;
		
		// Set Message Color
		storage->color = RGB_8888(0xFF, 0xD7, 0x00);
	}
	
	// User Chat Message
	else if(strcmp(nickname, "ME") != 0)
	{
		// Activate Notification
		shownotification = 1;
	}
}

// Add Message to Outbox
void addToOutbox(char * text)
{
	// Other Player online
	if(usercount > 1)
	{
		// Find free Outbox Slot
		int i = 0; for(; i < OUTBOX_SIZE; i++)
		{
			// Found free Outbox Slot
			if(outbox[i][0] == 0)
			{
				// Link Field
				strncpy(outbox[i], text, CHAT_TRUNCATE);
				
				// Stop Search
				break;
			}
		}
	}
}

// Pop Message from Outbox
int popFromOutbox(char * buffer)
{
	// Find filled Outbox Slot
	int i = 0; for(; i < OUTBOX_SIZE; i++)
	{
		// Found used Outbox Slot
		if(outbox[i][0] != 0)
		{
			// Copy String
			strcpy(buffer, outbox[i]);
			
			// Clear Slot
			memset(outbox[i], 0, sizeof(outbox[i]));
			
			// Return Success
			return 1;
		}
	}
	
	// Nothing found
	return 0;
}

// Set Connection Status
void setConnectionStatus(int status)
{
	// Save Status
	connected_to_prometheus = status;
	
	// Disconnected
	if(!connected_to_prometheus)
	{
		// Clear Outbox
		memset(outbox, 0, sizeof(outbox));
	}
}

// Set WLAN Reception Percentage
void setReceptionPercentage(int percentage)
{
	// Save Percentage
	reception = percentage;
}

// Set User Count
void setUserCount(int count)
{
	// Save User Count
	usercount = count;
}

// Handle Key Event
void handleKeyEvent(uint32_t prev, uint32_t curr)
{
	// Online Mode
	if(onlinemode)
	{
		// Connected to Server
		if(connected_to_prometheus)
		{
			// Select Down
			if(!is_delete_button_pressed && (prev & PSP_CTRL_SELECT) == 0 && (curr & PSP_CTRL_SELECT) != 0)
			{
				// Activate Delete Button Press Flag
				is_delete_button_pressed = 1;
				
				// Start Press Timer
				sceKernelGetSystemTime(&delete_press_start);
				
				// Clone Data to End Press Timer
				delete_press_end = delete_press_start;
			}
			
			// Select Up
			else if(is_delete_button_pressed && (prev & PSP_CTRL_SELECT) != 0 && (curr & PSP_CTRL_SELECT) == 0)
			{
				// Reset Press Flag
				is_delete_button_pressed = 0;
				
				// Long Press Time not reached (< 1s)
				if((delete_press_end.low - delete_press_start.low) < 1000000)
				{
					// Letter available in Chat Line
					if(strlen(chatline) > 0)
					{
						// Delete Last Letter
						chatline[strlen(chatline) - 1] = 0;
					}
				}
			}
			
			// Select Press Update
			else if(is_delete_button_pressed && (prev & PSP_CTRL_SELECT) != 0 && (curr & PSP_CTRL_SELECT) != 0)
			{
				// Update End Timer
				sceKernelGetSystemTime(&delete_press_end);
				
				// Surpassing the 2 Second Limit
				if((delete_press_end.low - delete_press_start.low) >= 2000000)
				{
					// Clear Chat Line
					chatline[0] = 0;
				}
			}
			
			// Start Down
			if(!is_send_button_pressed && (prev & PSP_CTRL_START) == 0 && (curr & PSP_CTRL_START) != 0)
			{
				// Activate Send Button Press Flag
				is_send_button_pressed = 1;
				
				// Start Press Timer
				sceKernelGetSystemTime(&send_press_start);
				
				// Clone Data to End Press Timer
				send_press_end = send_press_start;
			}
			
			// Start Up
			else if(is_send_button_pressed && (prev & PSP_CTRL_START) != 0 && (curr & PSP_CTRL_START) == 0)
			{
				// Reset Press Flag
				is_send_button_pressed = 0;
				
				// Long Press Time not reached (< 1s)
				if((send_press_end.low - send_press_start.low) < 1000000)
				{
					// Message in Writing Queue
					if(strlen(chatline) > 0)
					{
						// Schedule Chat Message for Sending via Adhoc Control Emulator
						addToOutbox(chatline);
						
						// Add to own Chat Log
						addChatLog("ME", chatline);
						
						// Clear Chat Line
						chatline[0] = 0;
					}
				}
			}
			
			// Start Press Update
			else if(is_send_button_pressed && (prev & PSP_CTRL_START) != 0 && (curr & PSP_CTRL_START) != 0)
			{
				// Update End Timer
				sceKernelGetSystemTime(&send_press_end);
			}
			
			// Keyphrase Page
			if((curr & PSP_CTRL_LTRIGGER) != 0 && (curr & PSP_CTRL_RTRIGGER) != 0) phrasepackage = 3;
			else if((curr & PSP_CTRL_RTRIGGER) != 0) phrasepackage = 2;
			else if((curr & PSP_CTRL_LTRIGGER) != 0) phrasepackage = 1;
			else phrasepackage = 0;
			
			// Key Phrase Index
			int keyphraseindex = -1;
			
			// Calculate Page-Relative Index
			// UP, LEFT, DOWN, RIGHT, TRIANGLE, SQUARE, CROSS, CIRCLE
			if((prev & PSP_CTRL_UP) == 0 && (curr & PSP_CTRL_UP) != 0) keyphraseindex = 0;
			else if((prev & PSP_CTRL_LEFT) == 0 && (curr & PSP_CTRL_LEFT) != 0) keyphraseindex = 1;
			else if((prev & PSP_CTRL_DOWN) == 0 && (curr & PSP_CTRL_DOWN) != 0) keyphraseindex = 2;
			else if((prev & PSP_CTRL_RIGHT) == 0 && (curr & PSP_CTRL_RIGHT) != 0) keyphraseindex = 3;
			else if((prev & PSP_CTRL_TRIANGLE) == 0 && (curr & PSP_CTRL_TRIANGLE) != 0) keyphraseindex = 4;
			else if((prev & PSP_CTRL_SQUARE) == 0 && (curr & PSP_CTRL_SQUARE) != 0) keyphraseindex = 5;
			else if((prev & PSP_CTRL_CROSS) == 0 && (curr & PSP_CTRL_CROSS) != 0) keyphraseindex = 6;
			else if((prev & PSP_CTRL_CIRCLE) == 0 && (curr & PSP_CTRL_CIRCLE) != 0) keyphraseindex = 7;
			
			// Valid Page-Relative Index
			if(keyphraseindex >= 0)
			{
				// Calculate Absolute Index
				keyphraseindex += BUTTON_MAPPINGS_ON_SCREEN * phrasepackage;
				
				// Add Letter to Text
				if(keyphraseindex < 31 && strlen(chatline) < CHAT_TRUNCATE) strcpy(chatline + strlen(chatline), keyphrases[keyphraseindex]);
				
				// Put Space Symbol
				else if(keyphraseindex == 31 && strlen(chatline) < CHAT_TRUNCATE) strcpy(chatline + strlen(chatline), " ");
			}
		}
	}
}
