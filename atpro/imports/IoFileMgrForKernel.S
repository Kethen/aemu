/*
    .set noreorder

#include "pspimport.s"

	//IMPORT_START "IoFileMgrForKernel",0x00010000
	IMPORT_FUNC  "IoFileMgrForKernel",0x18881E58,sceIoGetFdDebugInfo


	.set noreorder
*/

#include "pspstub.s"
	// https://github.com/uofw/uofw/blob/master/src/kd/iofilemgr/exports.exp

	STUB_START	"IoFileMgrForKernel",0x00010011,0x00010005
	STUB_FUNC	0x18881E58,sceIoGetFdDebugInfo
	STUB_END
