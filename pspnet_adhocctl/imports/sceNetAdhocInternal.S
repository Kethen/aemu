	.set noreorder

#include "pspstub.s"

	STUB_START "sceNetAdhocInternal",0x00090011,0x00010005
	STUB_FUNC  0x1,resolve_server_ip
	STUB_END
