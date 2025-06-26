	.set noreorder

#include "pspstub.s"

	STUB_START "sceNetMiniUPnP",0x00090011,0x00020005
	STUB_FUNC  0x1,miniupnc_start
	STUB_FUNC  0x2,miniupnc_stop
	STUB_END
