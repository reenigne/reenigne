	.section .init
	.global	_start
_start:
	.word 0
    .word .module_init - _start
    .word .module_finalize - _start
    .word module_service_call_handler - _start
    .word module_title - _start
    .word module_help_string - _start
    .word module_help_table - _start

.module_init:
	ldr r0, [r12]
	cmp	r0, #0
	bne .got_workspace
	mov	r0, #6  @ Claim
    mov r3, #4  @ sizeof(workspace)
    swi #0x1e   @ OS_Module
    orrvs pc,r14,#0x10000000
    mov r0, r2
    str r0, [r12]
.got_workspace:
    br moduleInit

.module_finalize:
    ldr r0, [r12]
    br moduleFinalize

.module_service_call_handler:
    ldr r0, [r12]
    br moduleServiceCallHandler

module_title:
    .ascii "SerialFS\000";
module_help_string:
    .ascii "SerialFS	0.00 (08 Sep 2015) [http://www.reenigne.org/serialfs]\000";
module_help_table:
    .ascii "SerialFS\000";
    .align 4
    .word oscli_serialfs - _start
    .word 0       @ Minimum parameters = 0, OS_GSTrans map = 0, Maximum parameters = 0, FS command = 0, *configure = 0, is_code = 0

