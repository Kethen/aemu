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

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsysmem_kernel.h>
#include <psputilsforkernel.h>
#include <pspmoduleexport.h>
#include <psploadcore.h>
#include <pspsdk.h>
#include <stdio.h>
#include <string.h>

#include "libs.h"
#include "systemctrl.h"

PspModuleImport * find_import_lib(SceModule *pMod, char *library)
{
	PspModuleImport *pImp;
	void *stubTab;
	int stubLen;
	int i = 0;

	if(pMod == NULL)
		return NULL;

	stubTab = pMod->stub_top;
	stubLen = pMod->stub_size;
	while(i<stubLen) {
		pImp = (PspModuleImport*)(stubTab+i);
		if((pImp->name) && (strcmp(pImp->name, library) == 0))
			return pImp;
		i += (pImp->entLen * 4);
	}

	return NULL;
}


unsigned int find_import_bynid(SceModule *pMod, char *library, unsigned int nid)
{
	PspModuleImport *pImp;
	int i;

	pImp = find_import_lib(pMod, library);
	if(pImp) {
		for(i=0; i<pImp->funcCount; i++) {
			if(pImp->fnids[i] == nid)
				return (unsigned int) &pImp->funcs[i*2];
		}
	}

	return 0;
}

int hook_global_module_jal(SceModule2 * mod, uint32_t oldcall, uint32_t newcall)
{
	// Invalid Arguments
	if(mod == NULL)
		return -1;
	
	// Hook Counter
	int counter = 0;
	
	// Machine Code of Module
	uint32_t * asmtext = (uint32_t *)mod->text_addr;
	
	// Machine Code Segment Size
	uint32_t asmsize = mod->text_size;
	
	// ASM Iterator
	uint32_t i = 0;
	
	// Iterate Machine Code
	for(; i < asmsize / 4; i ++)
	{
		// Get Instruction
		uint32_t inst = asmtext[i];
		
		// JAL Instruction
		if((inst & 0xFC000000) == 0x0C000000)
		{
			// Matching JAL found
			if(GET_CALL_ADDR(inst) == oldcall)
			{
				// Overwrite JAL
				asmtext[i] = MAKE_CALL(newcall);
				
				// Log Overwrite
				counter++;
			}
		}
	}
	
	// Hooked Calls
	if(counter > 0)
	{
		// Synchronize Cache
		sceKernelDcacheWritebackAll();
		sceKernelIcacheInvalidateAll();
	}
	
	// Return Hook Counter
	return counter;
}

int hook_weak_user_bynid(SceModule2 * mod, char * library, unsigned int nid, void * func)
{
	// Invalid Arguments
	if(mod == NULL || library == NULL || func == NULL)
		return -1;
	
	// Stub Table
	void * stubTab = mod->stub_top;
	uint32_t stubLen = mod->stub_size;
	
	// Stub Iterator
	uint32_t i = 0;
	
	// Iterate Stubs
	while(i < stubLen)
	{
		// Grab Module Import
		PspModuleImport * import = (PspModuleImport *)(stubTab + i);
		
		// Targeted Import found
		if(import->name != NULL && strcmp(import->name, library) == 0)
		{
			// Function Iterator
			uint32_t j = 0;
			
			// Iterate Functions
			for(; j < import->funcCount; j++)
			{
				// Targeted Function found
				if(import->fnids[j] == nid)
				{
					// Fetch Stub Address
					uint32_t address = (uint32_t)&import->funcs[j * 2];
					
					// Patch Calls to Weak Stub
					return hook_global_module_jal(mod, address, (uint32_t)func);
				}
			}
		}
		
		// Move Pointer
		i += import->entLen * 4;
	}
	
	// No calls patched
	return 0;
}

/**
 * Remember you have to export the hooker function if using syscall hook
 */
int hook_import_bynid(SceModule *pMod, char *library, unsigned int nid, void *func)
{
	PspModuleImport *pImp;
	void *stubTab;
	int stubLen;
	int i = 0;

	if(pMod == NULL)
		return -1;

	stubTab = pMod->stub_top;
	stubLen = pMod->stub_size;

	while(i<stubLen) {
		pImp = (PspModuleImport*)(stubTab+i);

		if((pImp->name) && (strcmp(pImp->name, library) == 0)) {
			int j;

			for(j=0; j<pImp->funcCount; j++) {
				if(pImp->fnids[j] == nid) {
					void *addr = (void*)(&pImp->funcs[j*2]);
					if(func == NULL) {
						_sw(0x03E00008, (u32)addr);
						_sw(NOP, (u32)(addr + 4));
					} else {
						// Partition Check
						SceModule2 * mod = (SceModule2 *)pMod;
						if(((mod->text_addr & 0x80000000) && (((uint32_t)func) & 0x80000000)) || ((mod->text_addr & 0x80000000) == 0 && (((uint32_t)func) & 0x80000000) == 0))
						{
							REDIRECT_FUNCTION(func, addr);
						} else {
							_sw(0x03E00008, (u32)addr);
							_sw(MAKE_SYSCALL(sctrlKernelQuerySystemCall(func)), (u32)(addr + 4));
						}
					}

					// *I think* these has to be aligned, let's just flush all
					//sceKernelDcacheWritebackInvalidateRange(addr, 8);
					//sceKernelIcacheInvalidateRange(addr, 8);
					sceKernelDcacheWritebackAll();
					sceKernelIcacheInvalidateAll();
				}
			}
		}

		i += (pImp->entLen * 4);
	}

	return 0;
}
