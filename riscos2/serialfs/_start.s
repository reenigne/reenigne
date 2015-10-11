	.section .init
	.global	_start
    .global filing_system_information_block
    .global filing_system_name
_start:
	.word 0
    .word .module_init - _start
    .word .module_finalize - _start
    .word .module_service_call_handler - _start
    .word .module_title - _start
    .word .module_help_string - _start
    .word .module_help_table - _start

.module_init:
	ldr r0, [r12]
	cmp	r0, #0
	bne .got_workspace
	mov	r0, #6    @ Claim
    mov r3, #4    @ sizeof(workspace)
    swi #0x2001e  @ XOS_Module
    orrvs pc, r14, #0x10000000   @ oVerflow flag
    str r2, [r12]
.got_workspace:
    mov r0, #12   @ Add a filing system
.t1:
    sub r1, pc, #(.t1 + 8) - _start
.t2:
    sub r2, pc, #(.t2 + 8) - filing_system_information_block
    mov r3, r12
    swi #0x20029  @ XOS_FSControl
    orrvs pc, r14, #0x10000000   @ oVerflow flag
    ldr r0, [r12]
    b moduleInit

.module_finalize:
    mov r0, #16   @ RemoveFS
.t3:
    sub r1, pc, #(.t3 + 8) - filing_system_name
    swi #0x20029  @ XOS_FSControl
    @ errors ignored deliberately
    ldr r0, [r12]
    b moduleFinalize

.module_service_call_handler:
    ldr r0, [r12]
    b moduleServiceCallHandler

.oscli_serialfs:
    mov r0, #16   @ SelectFS
.t4:
    add r1, pc, #(.t4 + 8) - filing_system_name
    swi #0x20029  @ XOS_FSControl
    orrvs pc, r14, #0x10000000   @ oVerflow flag
    ldr r0, [r12]
    b oscliSerialFS

.fsentry_open:
    ldr r0, [r12]
    b fsEntryOpen

.fsentry_getbytes:
    ldr r0, [r12]
    b fsEntryGetBytes

.fsentry_putbytes:
    ldr r0, [r12]
    b fsEntryPutBytes

.fsentry_args:
    ldr r0, [r12]
    b fsEntryArgs

.fsentry_close:
    ldr r0, [r12]
    b fsEntryClose

.fsentry_file:
    ldr r0, [r12]
    b fsEntryFile

.fsentry_func:
    ldr r0, [r12]
    b fsEntryFunc

.fsentry_gbpb:
    ldr r0, [r12]
    b fsEntryGBPB

.module_title:
    .ascii "SerialFS\000"
.module_help_string:
    .ascii "SerialFS	0.00 (08 Sep 2015) [http://www.reenigne.org/serialfs]\000"
.module_help_table:
    .ascii "SerialFS\000"
    .align 2
    .word .oscli_serialfs - _start
    .word 0       @ Minimum parameters = 0, OS_GSTrans map = 0, Maximum parameters = 0, FS command = 0, *configure = 0, is_code = 0

filing_system_information_block:
    .word filing_system_name
    .word .filing_system_startup_text
    .word .fsentry_open
    .word .fsentry_getbytes
    .word .fsentry_putbytes
    .word .fsentry_args
    .word .fsentry_close
    .word .fsentry_file
    .word 0xf0   @ filing system information word - TODO: allocate filing system number via https://www.riscosopen.org/content/allocate
    .word .fsentry_func
    .word .fsentry_gbpb

filing_system_name:
    .ascii "SerialFS\000"

.filing_system_startup_text:
    .ascii "\000"


