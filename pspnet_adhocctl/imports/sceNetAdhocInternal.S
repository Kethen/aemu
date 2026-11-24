	.set noreorder

#include "pspstub.s"

	STUB_START "sceNetAdhocInternal",0x00090011,0x00020005
	STUB_FUNC  0x1,resolve_server_ip
	STUB_FUNC  0x4,atpro_profile_refresh
	STUB_END
