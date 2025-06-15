    .set noreorder

#include "pspimport.s"
	// https://github.com/uofw/uofw/blob/master/src/kd/sysmem/exports.exp

	IMPORT_START "sceSuspendForKernel",0x40010000
	IMPORT_FUNC  "sceSuspendForKernel",0x3E0271D3,sceKernelVolatileMemLock
	IMPORT_FUNC  "sceSuspendForKernel",0xA14F40B2,sceKernelVolatileMemTryLock
	IMPORT_FUNC  "sceSuspendForKernel",0xA569E425,sceKernelVolatileMemUnlock
	IMPORT_FUNC  "sceSuspendForKernel",0xEADB1BD7,sceKernelPowerLock
	IMPORT_FUNC  "sceSuspendForKernel",0x3AEE7261,sceKernelPowerUnlock
	IMPORT_FUNC  "sceSuspendForKernel",0xB53B2147,sceKernelPowerLockForUser
	IMPORT_FUNC  "sceSuspendForKernel",0xC7C928C7,sceKernelPowerUnlockForUser
