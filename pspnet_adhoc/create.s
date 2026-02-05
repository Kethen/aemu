	.file	1 "create.c"
	.section .mdebug.eabi32
	.previous
	.section .gcc_compiled_long32
	.previous
	.nan	legacy
	.module	singlefloat
	.module	nooddspreg
	.module	arch=allegrex
	.text
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
$LC1:
	.ascii	"ms0:/seplugins/server.txt\000"
	.align	2
$LC2:
	.ascii	"ms0:/PSP/PLUGINS/atpro/server.txt\000"
	.text
	.align	2
	.set	nomips16
	.set	nomicromips
	.ent	resolve_server_ip.part.0
	.type	resolve_server_ip.part.0, @function
resolve_server_ip.part.0:
	.frame	$sp,664,$31		# vars= 648, regs= 3/0, args= 0, gp= 0
	.mask	0x80030000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-664
	sw	$31,660($sp)
	sw	$17,656($sp)
	jal	sceNetResolverInit
	sw	$16,652($sp)

	beq	$2,$0,$L2
	li	$6,512			# 0x200

	lw	$4,%gp_rel(_server_resolve_mutex)($28)
$L21:
	jal	sceKernelSignalSema
	li	$5,1			# 0x1

	li	$2,-1			# 0xffffffffffffffff
$L1:
	lw	$31,660($sp)
	lw	$17,656($sp)
	lw	$16,652($sp)
	jr	$31
	addiu	$sp,$sp,664

$L2:
	move	$5,$sp
	addiu	$4,$sp,644
	jal	sceNetResolverCreate
	sw	$0,644($sp)

	beql	$2,$0,$L4
	lui	$4,%hi($LC1)

$L20:
	jal	sceNetResolverTerm
	nop

	b	$L21
	lw	$4,%gp_rel(_server_resolve_mutex)($28)

$L4:
	li	$6,511			# 0x1ff
	li	$5,1			# 0x1
	jal	sceIoOpen
	addiu	$4,$4,%lo($LC1)

	bgez	$2,$L6
	move	$16,$2

	lui	$4,%hi($LC2)
	li	$6,511			# 0x1ff
	li	$5,1			# 0x1
	jal	sceIoOpen
	addiu	$4,$4,%lo($LC2)

	bgez	$2,$L6
	move	$16,$2

	lw	$4,644($sp)
$L17:
	jal	sceNetResolverDelete
	nop

	b	$L20
	nop

$L6:
	li	$6,128			# 0x80
	move	$5,$0
	jal	memset
	addiu	$4,$sp,512

	addiu	$5,$sp,512
	li	$6,127			# 0x7f
	jal	sceIoRead
	move	$4,$16

	move	$17,$2
	jal	sceIoClose
	move	$4,$16

	lw	$4,644($sp)
	addiu	$3,$sp,512
	bltz	$17,$L17
	addiu	$5,$sp,639

	li	$6,13			# 0xd
	li	$7,10			# 0xa
	lb	$2,0($3)
$L25:
	bne	$2,$0,$L8
	nop

	li	$7,458752			# 0x70000
$L22:
	li	$8,2			# 0x2
	ori	$7,$7,0xa120
	addiu	$6,$sp,640
	addiu	$5,$sp,512
	jal	sceNetResolverStartNtoA
	sw	$0,640($sp)

	lw	$4,644($sp)
	jal	sceNetResolverDelete
	move	$16,$2

	jal	sceNetResolverTerm
	nop

	beql	$16,$0,$L23
	lw	$2,640($sp)

	addiu	$5,$sp,640
	jal	sceNetInetInetAton
	addiu	$4,$sp,512

	lw	$2,640($sp)
$L23:
	lw	$4,%gp_rel(_server_resolve_mutex)($28)
	li	$5,1			# 0x1
	jal	sceKernelSignalSema
	sw	$2,%gp_rel(server_ip)($28)

	b	$L1
	lw	$2,%gp_rel(server_ip)($28)

$L8:
	beql	$2,$6,$L12
	sb	$0,0($3)

	bnel	$2,$7,$L24
	addiu	$3,$3,1

	sb	$0,0($3)
$L12:
	addiu	$3,$3,1
$L24:
	bnel	$3,$5,$L25
	lb	$2,0($3)

	b	$L22
	li	$7,458752			# 0x70000

	.set	macro
	.set	reorder
	.end	resolve_server_ip.part.0
	.size	resolve_server_ip.part.0, .-resolve_server_ip.part.0
	.align	2
	.globl	resolve_server_ip
	.set	nomips16
	.set	nomicromips
	.ent	resolve_server_ip
	.type	resolve_server_ip, @function
resolve_server_ip:
	.frame	$sp,8,$31		# vars= 0, regs= 1/0, args= 0, gp= 0
	.mask	0x80000000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lw	$4,%gp_rel(_server_resolve_mutex)($28)
	addiu	$sp,$sp,-8
	move	$6,$0
	sw	$31,4($sp)
	jal	sceKernelWaitSema
	li	$5,1			# 0x1

	lw	$2,%gp_rel(server_ip)($28)
	bne	$2,$0,$L30
	lw	$31,4($sp)

	j	resolve_server_ip.part.0
	addiu	$sp,$sp,8

$L30:
	lw	$4,%gp_rel(_server_resolve_mutex)($28)
	jal	sceKernelSignalSema
	li	$5,1			# 0x1

	lw	$31,4($sp)
	lw	$2,%gp_rel(server_ip)($28)
	jr	$31
	addiu	$sp,$sp,8

	.set	macro
	.set	reorder
	.end	resolve_server_ip
	.size	resolve_server_ip, .-resolve_server_ip
	.align	2
	.globl	__builtin_bswap16
	.set	nomips16
	.set	nomicromips
	.ent	__builtin_bswap16
	.type	__builtin_bswap16, @function
__builtin_bswap16:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	wsbh	$2,$4
	jr	$31
	andi	$2,$2,0xffff

	.set	macro
	.set	reorder
	.end	__builtin_bswap16
	.size	__builtin_bswap16, .-__builtin_bswap16
	.align	2
	.globl	_IsLocalMAC
	.set	nomips16
	.set	nomicromips
	.ent	_IsLocalMAC
	.type	_IsLocalMAC, @function
_IsLocalMAC:
	.frame	$sp,16,$31		# vars= 8, regs= 2/0, args= 0, gp= 0
	.mask	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	addiu	$sp,$sp,-16
	sw	$16,8($sp)
	move	$16,$4
	sw	$31,12($sp)
	jal	sceNetGetLocalEtherAddr
	move	$4,$sp

	move	$5,$sp
	move	$4,$16
	jal	memcmp
	li	$6,6			# 0x6

	lw	$31,12($sp)
	lw	$16,8($sp)
	sltu	$2,$2,1
	jr	$31
	addiu	$sp,$sp,16

	.set	macro
	.set	reorder
	.end	_IsLocalMAC
	.size	_IsLocalMAC, .-_IsLocalMAC
	.align	2
	.globl	_IsPDPPortInUse
	.set	nomips16
	.set	nomicromips
	.ent	_IsPDPPortInUse
	.type	_IsPDPPortInUse, @function
_IsPDPPortInUse:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lui	$2,%hi(_sockets)
	addiu	$2,$2,%lo(_sockets)
	addiu	$5,$2,1020
	lw	$3,0($2)
$L45:
	beql	$3,$0,$L44
	addiu	$2,$2,4

	lbu	$6,0($3)
	bnel	$6,$0,$L44
	addiu	$2,$2,4

	lbu	$6,15($3)
	lbu	$3,16($3)
	sll	$3,$3,8
	or	$3,$3,$6
	beq	$3,$4,$L39
	nop

	addiu	$2,$2,4
$L44:
	bnel	$2,$5,$L45
	lw	$3,0($2)

	jr	$31
	move	$2,$0

$L39:
	jr	$31
	li	$2,1			# 0x1

	.set	macro
	.set	reorder
	.end	_IsPDPPortInUse
	.size	_IsPDPPortInUse, .-_IsPDPPortInUse
	.section	.rodata.str1.4
	.align	2
$LC3:
	.ascii	"UDP\000"
	.section	.sdata,"aw"
	.align	2
$LC0:
	.ascii	"\000\000"
	.space	4
	.text
	.align	2
	.globl	proNetAdhocPdpCreate
	.set	nomips16
	.set	nomicromips
	.ent	proNetAdhocPdpCreate
	.type	proNetAdhocPdpCreate, @function
proNetAdhocPdpCreate:
	.frame	$sp,64,$31		# vars= 32, regs= 8/0, args= 0, gp= 0
	.mask	0x807f0000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	lw	$2,%gp_rel(_init)($28)
	addiu	$sp,$sp,-64
	sw	$31,60($sp)
	sw	$22,56($sp)
	sw	$21,52($sp)
	sw	$20,48($sp)
	sw	$19,44($sp)
	sw	$18,40($sp)
	sw	$17,36($sp)
	beq	$2,$0,$L66
	sw	$16,32($sp)

	beql	$4,$0,$L88
	li	$16,-2143223808			# 0xffffffff80410000

	blez	$6,$L68
	move	$21,$6

	lw	$2,%gp_rel($LC0)($28)
	move	$18,$5
	addiu	$4,$sp,16
	sw	$2,16($sp)
	lhu	$2,%gp_rel($LC0+4)($28)
	jal	sceNetGetLocalEtherAddr
	sh	$2,20($sp)

	beq	$18,$0,$L48
	nop

	jal	_IsPDPPortInUse
	move	$4,$18

	beq	$2,$0,$L51
	li	$16,-2143223808			# 0xffffffff80410000

	b	$L46
	addiu	$16,$16,1802

$L50:
	jal	_getRandomNumber
	li	$4,65535			# 0xffff

	andi	$18,$2,0xffff
$L48:
	jal	_IsPDPPortInUse
	move	$4,$18

	bne	$2,$0,$L50
	nop

$L51:
	lw	$2,%gp_rel(_postoffice)($28)
	beq	$2,$0,$L53
	li	$6,17			# 0x11

	jal	resolve_server_ip
	nop

	sw	$2,0($sp)
	addiu	$7,$sp,24
	li	$2,-20118			# 0xffffffffffffb16a
	move	$6,$18
	addiu	$5,$sp,16
	move	$4,$sp
	sh	$2,4($sp)
	jal	pdp_create_v4
	sw	$0,24($sp)

	beq	$2,$0,$L59
	move	$19,$2

	jal	malloc
	li	$4,68			# 0x44

	bne	$2,$0,$L55
	move	$17,$2

$L83:
	jal	pdp_delete
	move	$4,$19

$L59:
	li	$16,-2143223808			# 0xffffffff80410000
$L86:
	addiu	$16,$16,1
$L46:
	lw	$31,60($sp)
$L85:
	lw	$22,56($sp)
	lw	$21,52($sp)
	lw	$20,48($sp)
	lw	$19,44($sp)
	lw	$18,40($sp)
	lw	$17,36($sp)
	move	$2,$16
	lw	$16,32($sp)
	jr	$31
	addiu	$sp,$sp,64

$L55:
	sb	$0,0($2)
	swl	$19,8($2)
	li	$6,6			# 0x6
	addiu	$5,$sp,16
	addiu	$4,$2,9
	jal	memcpy
	swr	$19,5($2)

	sb	$18,15($17)
	lw	$4,%gp_rel(_socket_mapper_mutex)($28)
	srl	$18,$18,8
	sb	$18,16($17)
	swl	$21,20($17)
	li	$5,1			# 0x1
	swr	$21,17($17)
	jal	sceKernelWaitSema
	move	$6,$0

	lui	$2,%hi(_sockets)
	addiu	$2,$2,%lo(_sockets)
	move	$16,$0
	move	$3,$2
	li	$5,255			# 0xff
$L58:
	lw	$6,0($2)
	move	$4,$16
	bne	$6,$0,$L57
	addiu	$16,$16,1

	sll	$2,$4,2
	addu	$3,$3,$2
	sw	$17,0($3)
	lw	$4,%gp_rel(_socket_mapper_mutex)($28)
$L87:
	jal	sceKernelSignalSema
	li	$5,1			# 0x1

	b	$L85
	lw	$31,60($sp)

$L57:
	bne	$16,$5,$L58
	addiu	$2,$2,4

	lw	$4,%gp_rel(_socket_mapper_mutex)($28)
	jal	sceKernelSignalSema
	li	$5,1			# 0x1

	jal	free
	move	$4,$17

	b	$L83
	nop

$L53:
	li	$5,2			# 0x2
	jal	sceNetInetSocket
	li	$4,2			# 0x2

	blez	$2,$L59
	move	$19,$2

	li	$8,4			# 0x4
	addiu	$7,$28,%gp_rel(_one)
	li	$6,4			# 0x4
	li	$5,65535			# 0xffff
	jal	sceNetInetSetsockopt
	move	$4,$2

	li	$8,4			# 0x4
	addiu	$7,$28,%gp_rel(_one)
	li	$6,512			# 0x200
	li	$5,65535			# 0xffff
	jal	sceNetInetSetsockopt
	move	$4,$19

	li	$2,528			# 0x210
	sh	$2,0($sp)
	li	$6,16			# 0x10
	wsbh	$2,$18
	move	$5,$sp
	move	$4,$19
	sw	$0,4($sp)
	jal	sceNetInetBind
	sh	$2,2($sp)

	bne	$2,$0,$L61
	move	$20,$2

	jal	malloc
	li	$4,68			# 0x44

	beq	$2,$0,$L61
	move	$17,$2

	li	$6,68			# 0x44
	move	$5,$0
	jal	memset
	move	$4,$2

	lw	$4,%gp_rel(_socket_mapper_mutex)($28)
	move	$6,$0
	jal	sceKernelWaitSema
	li	$5,1			# 0x1

	lui	$2,%hi(_sockets)
	addiu	$2,$2,%lo(_sockets)
	move	$22,$2
	li	$3,255			# 0xff
$L64:
	lw	$4,0($2)
	beq	$4,$0,$L63
	addiu	$16,$20,1

	move	$20,$16
	bne	$16,$3,$L64
	addiu	$2,$2,4

	lw	$4,%gp_rel(_socket_mapper_mutex)($28)
	jal	sceKernelSignalSema
	li	$5,1			# 0x1

	jal	free
	move	$4,$17

$L61:
	jal	sceNetInetClose
	move	$4,$19

	b	$L86
	li	$16,-2143223808			# 0xffffffff80410000

$L66:
	li	$16,-2143223808			# 0xffffffff80410000
	b	$L46
	addiu	$16,$16,1810

$L68:
	li	$16,-2143223808			# 0xffffffff80410000
$L88:
	b	$L46
	addiu	$16,$16,1809

$L63:
	swl	$19,8($17)
	addiu	$5,$sp,16
	addiu	$4,$17,9
	swr	$19,5($17)
	jal	memcpy
	li	$6,6			# 0x6

	srl	$2,$18,8
	sb	$18,15($17)
	sb	$2,16($17)
	sll	$20,$20,2
	swl	$21,20($17)
	lui	$4,%hi($LC3)
	addu	$22,$22,$20
	swr	$21,17($17)
	move	$5,$18
	addiu	$4,$4,%lo($LC3)
	jal	sceNetPortOpen
	sw	$17,0($22)

	b	$L87
	lw	$4,%gp_rel(_socket_mapper_mutex)($28)

	.set	macro
	.set	reorder
	.end	proNetAdhocPdpCreate
	.size	proNetAdhocPdpCreate, .-proNetAdhocPdpCreate
	.local	server_ip
	.comm	server_ip,4,4
	.ident	"GCC: (GNU) 14.2.0"
