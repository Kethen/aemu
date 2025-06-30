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
#include <pspiofilemgr.h>

#include <string.h>
#include <stdio.h>

#define MODULENAME "midmem_layout"
PSP_MODULE_INFO(MODULENAME, PSP_MODULE_KERNEL, 1, 6);

int log_fd = -1;

#ifdef DEBUG
#define LOG(...) { \
	if (log_fd < 0){ \
		log_fd = sceIoOpen("ms0:/PSP/" MODULENAME ".log", PSP_O_WRONLY | PSP_O_APPEND, 0777); \
		if (log_fd < 0){ \
			log_fd = sceIoOpen("ef0:/PSP/" MODULENAME ".log", PSP_O_WRONLY | PSP_O_APPEND, 0777); \
		} \
	} \
	if (log_fd >= 0){ \
		char _log_buf[128]; \
		int _log_len = sprintf(_log_buf, __VA_ARGS__); \
		sceIoWrite(log_fd, _log_buf, _log_len); \
		sceIoClose(log_fd); \
		log_fd = -1; \
	} \
}
#else
#define LOG(...)
#endif

typedef struct SceModule2 {
    struct SceModule2   *next;
    unsigned short      attribute;
    unsigned char       version[2];
    char                modname[27];
    char                terminal;
    unsigned int        unknown1;
    unsigned int        unknown2;
    SceUID              modid;
    unsigned int        unknown3[2];
    u32         mpid_text;  // 0x38
    u32         mpid_data; // 0x3C
    void *              ent_top;
    unsigned int        ent_size;
    void *              stub_top;
    unsigned int        stub_size;
    unsigned int        unknown4[5];
    unsigned int        entry_addr;
    unsigned int        gp_value;
    unsigned int        text_addr;
    unsigned int        text_size;
    unsigned int        data_size;
    unsigned int        bss_size;
    unsigned int        nsegment;
    unsigned int        segmentaddr[4];
    unsigned int        segmentsize[4];
} SceModule2;

int sctrlHENSetMemory(u32 p2, u32 p9);
typedef int (* STMOD_HANDLER)(SceModule2 *);
STMOD_HANDLER sctrlHENSetStartModuleHandler(STMOD_HANDLER handler);

STMOD_HANDLER prev;

#define TARGET_P2 32

int mod_start_handler(SceModule2 *mod){
	char name_buf[28] = {0};
	memcpy(name_buf, mod->modname, 27);
	LOG("%s: module %s\n", __func__, name_buf);


	//#define TARGET_MOD_NAME "sceMpegVsh_library"
	//#define TARGET_MOD_NAME "vsh_module"
	#define TARGET_MOD_NAME "impose_plugin_module"

	if (strcmp(mod->modname, TARGET_MOD_NAME) == 0){
		int set_status = sctrlHENSetMemory(TARGET_P2, 52 - TARGET_P2);
		LOG("%s: sctrlHENSetMemory status 0x%x\n", __func__, set_status);
	}

	if(prev != NULL){
		return prev(mod);
	}

	return 0;
}

// Module Start Event
int module_start(SceSize args, void * argp)
{
	#ifdef DEBUG
	log_fd = sceIoOpen("ms0:/PSP/" MODULENAME ".log", PSP_O_WRONLY | PSP_O_TRUNC | PSP_O_CREAT, 0777);
	if (log_fd < 0){
		log_fd = sceIoOpen("ef0:/PSP/" MODULENAME ".log", PSP_O_WRONLY | PSP_O_TRUNC | PSP_O_CREAT, 0777);
	}
	#endif

	LOG("%s: begin\n", __func__);

	prev = sctrlHENSetStartModuleHandler(mod_start_handler);
	return 0;
}

// Module Stop Event
int module_stop(SceSize args, void * argp)
{
	
	return 0;
}
