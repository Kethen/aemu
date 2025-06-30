    .set noreorder

#include "pspimport.s"

	IMPORT_START "SystemCtrlForKernel",0x40010000
	IMPORT_FUNC  "SystemCtrlForKernel",0x1C90BECB,sctrlHENSetStartModuleHandler
	IMPORT_FUNC  "SystemCtrlForKernel",0x745286D1,sctrlHENSetMemory

