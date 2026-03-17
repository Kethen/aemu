	.set noreorder

#include "pspimport.s"

	IMPORT_START "sceNetAdhocctlInternal",0x00090011
	IMPORT_FUNC  "sceNetAdhocctlInternal",0x80C0A7AB,_resolveIP
	IMPORT_FUNC  "sceNetAdhocctlInternal",0x0E71BEA5,_resolveMAC
	IMPORT_FUNC  "sceNetAdhocctlInternal",0x33453B4E,_getInternalPeerList
	IMPORT_FUNC "sceNetAdhocctlInternal",0x1,fix_game_mac
