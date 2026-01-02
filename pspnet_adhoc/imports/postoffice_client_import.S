	.set noreorder

#include "pspstub.s"

	STUB_START "aemu_postoffice_client",0x00090011,0x00100005
	STUB_FUNC  0x1,pdp_create_v4
	STUB_FUNC  0x2,pdp_delete
	STUB_FUNC  0x3,pdp_send
	STUB_FUNC  0x4,pdp_recv
	STUB_FUNC  0x5,pdp_get_native_sock
	STUB_FUNC  0x6,pdp_peek_next_size

	STUB_FUNC  0x101,ptp_listen_v4
	STUB_FUNC  0x102,ptp_accept
	STUB_FUNC  0x103,ptp_connect_v4
	STUB_FUNC  0x104,ptp_send
	STUB_FUNC  0x105,ptp_recv
	STUB_FUNC  0x106,ptp_close
	STUB_FUNC  0x107,ptp_listen_close
	STUB_FUNC  0x108,ptp_get_native_sock
	STUB_FUNC  0x109,ptp_listen_get_native_sock
	STUB_FUNC  0x10A,ptp_peek_next_size
	STUB_END
