	.set noreorder

#include "pspimport.s"

IMPORT_START "inet_redirect",0x00090000
IMPORT_FUNC "inet_redirect",0x201,rehook_inet
