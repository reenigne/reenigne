	.file	"v2c.c"
__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__CCP__  = 0x34
__tmp_reg__ = 0
__zero_reg__ = 1
	.global __do_copy_data
	.global __do_clear_bss
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
.global	encodeHex
	.type	encodeHex, @function
encodeHex:
.LFB10:
.LM1:
.LVL0:
/* prologue: function */
/* frame size = 0 */
.LM2:
	cpi r24,lo8(10)
	brlo .L6
.LM3:
	subi r24,lo8(-(55))
.LVL1:
.LM4:
	ret
.LVL2:
.L6:
.LM5:
	subi r24,lo8(-(48))
.LVL3:
	ret
.LFE10:
	.size	encodeHex, .-encodeHex
.global	sendNextByte
	.type	sendNextByte, @function
sendNextByte:
.LFB11:
.LM6:
/* prologue: function */
/* frame size = 0 */
.LM7:
	lds r24,flags2
	sbrs r24,2
	rjmp .L24
.LM8:
	lds r24,sendState
	cpi r24,lo8(2)
	breq .L10
	cpi r24,lo8(3)
	breq .L11
	cpi r24,lo8(1)
	breq .L25
.L24:
	ret
.L10:
.LM9:
	lds r24,debugValue
	andi r24,lo8(15)
.LBB145:
.LBB146:
.LM10:
	cpi r24,lo8(10)
	brlo .+2
	rjmp .L14
.LM11:
	subi r24,lo8(-(48))
.L15:
.LBE146:
.LBE145:
.LM12:
	sts 198,r24
.LM13:
	sts sendState,__zero_reg__
	ret
.L25:
.LM14:
	lds r24,debugValue
	swap r24
	andi r24,lo8(15)
.LBB148:
.LBB149:
.LM15:
	cpi r24,lo8(10)
	brlo .+2
	rjmp .L12
.LM16:
	subi r24,lo8(-(48))
.L13:
.LBE149:
.LBE148:
.LM17:
	sts 198,r24
.LM18:
	ldi r24,lo8(2)
	sts sendState,r24
	ret
.L11:
.LBB151:
.LM19:
.LVL4:
.LBE151:
	sbrs r17,6
	rjmp .L16
.LBB152:
.LM20:
.LVL5:
.LBE152:
	mov r30,r15
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	lds r24,serialLED
	mov r31,r30
	clr r30
	add r30,r24
	adc r31,__zero_reg__
	subi r30,lo8(-(waterBuffers))
	sbci r31,hi8(-(waterBuffers))
	ld r24,Z
	asr r24
	sts debugValue,r24
.LBB153:
.LM21:
.LVL6:
.LBE153:
	mov r30,r10
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	lds r24,serialLED
	mov r31,r30
	clr r30
	add r30,r24
	adc r31,__zero_reg__
	subi r30,lo8(-(lightBuffers))
	sbci r31,hi8(-(lightBuffers))
	ld r24,Z
	tst r24
	breq .L17
.LM22:
	lds r24,serialLED
.LBB154:
.LVL7:
.LBE154:
	ldi r25,lo8(0)
	andi r24,lo8(15)
	andi r25,hi8(15)
	mov r30,r9
	ldi r31,lo8(0)
	cp r24,r30
	cpc r25,r31
	brne .+2
	rjmp .L26
.LM23:
	lds r24,debugValue
	subi r24,lo8(-(10))
	sts debugValue,r24
.L17:
.LM24:
	lds r24,debugValue
	cpi r24,lo8(15)
	brlo .L19
	lds r24,debugValue
	sbrc r24,7
	rjmp .L19
.LM25:
	ldi r24,lo8(15)
	sts debugValue,r24
.L19:
.LM26:
	lds r24,debugValue
	sbrc r24,7
	rjmp .L27
.LVL8:
.L20:
.LM27:
	lds r24,debugValue
.LVL9:
.LBB155:
.LBB156:
.LM28:
	cpi r24,lo8(10)
	brsh .L21
.LM29:
	subi r24,lo8(-(48))
.L22:
.LBE156:
.LBE155:
.LM30:
	sts 198,r24
.LM31:
	lds r24,serialLED
	subi r24,lo8(-(1))
	sts serialLED,r24
.LM32:
	lds r24,serialLED
	tst r24
	breq .L28
.LM33:
	ldi r24,lo8(3)
	sts sendState,r24
	ret
.LVL10:
.L16:
.LM34:
	lds r30,serialLED
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
.LVL11:
	sts debugValue,r24
	rjmp .L20
.LVL12:
.L21:
.LBB158:
.LBB157:
.LM35:
	subi r24,lo8(-(55))
	rjmp .L22
.LVL13:
.L14:
.LBE157:
.LBE158:
.LBB159:
.LBB147:
	subi r24,lo8(-(55))
	rjmp .L15
.L12:
.LBE147:
.LBE159:
.LBB160:
.LBB150:
	subi r24,lo8(-(55))
	rjmp .L13
.LVL14:
.L28:
.LBE150:
.LBE160:
.LM36:
	sts sendState,__zero_reg__
	ret
.L27:
.LM37:
	lds r24,debugValue
.LM38:
	sts debugValue,__zero_reg__
	rjmp .L20
.L26:
.LM39:
	ldi r24,lo8(15)
	sts debugValue,r24
	rjmp .L17
.LFE11:
	.size	sendNextByte, .-sendNextByte
.global	decodeHex
	.type	decodeHex, @function
decodeHex:
.LFB12:
.LM40:
.LVL15:
/* prologue: function */
/* frame size = 0 */
	mov r25,r24
.LM41:
	subi r24,lo8(-(-48))
.LVL16:
	cpi r24,lo8(10)
	brlo .L30
.LM42:
	subi r24,lo8(-(-17))
	cpi r24,lo8(6)
	brlo .L34
.LM43:
	mov r24,r25
	subi r24,lo8(-(-97))
	cpi r24,lo8(6)
	brsh .L35
.LM44:
	mov r24,r25
	subi r24,lo8(-(-87))
.L30:
.LM45:
	ret
.L34:
.LM46:
	subi r24,lo8(-(10))
	ret
.L35:
.LM47:
	ldi r24,lo8(-1)
	ret
.LFE12:
	.size	decodeHex, .-decodeHex
.global	__vector_18
	.type	__vector_18, @function
__vector_18:
.LFB13:
.LM48:
	push __zero_reg__
	push r0
	in r0,__SREG__
	push r0
	clr __zero_reg__
	push r24
	push r25
	push r26
	push r27
	push r30
	push r31
/* prologue: Signal */
/* frame size = 0 */
.LM49:
	lds r25,198
.LVL17:
.LBB161:
.LBB163:
.LM50:
	mov r30,r25
.LVL18:
	subi r30,lo8(-(-48))
	cpi r30,lo8(10)
	brlo .L37
.LM51:
	mov r24,r25
	subi r24,lo8(-(-65))
	cpi r24,lo8(6)
	brsh .L38
.LM52:
	subi r30,lo8(-(-7))
.L37:
.LBE163:
.LBE161:
.LM53:
	lds r24,receiveState
	cpi r24,lo8(2)
	breq .L43
.L65:
	cpi r24,lo8(3)
	brlo .L59
	cpi r24,lo8(4)
	brne .+2
	rjmp .L45
	cpi r24,lo8(4)
	brlo .+2
	rjmp .L60
.LM54:
	cpi r25,lo8(63)
	brne .+2
	rjmp .L61
.LM55:
	cpi r25,lo8(61)
	brne .+2
	rjmp .L62
.L57:
.LM56:
	sts receiveState,__zero_reg__
	rjmp .L58
.L59:
.LM57:
	tst r24
	brne .L63
.LM58:
	cpi r30,lo8(-1)
	brne .+2
	rjmp .L48
.LM59:
	mov r25,r30
.LVL19:
	ldi r24,lo8(0)
.LVL20:
	sts (debugAddress)+1,r25
	sts debugAddress,r24
.LM60:
	ldi r24,lo8(1)
.LVL21:
	sts receiveState,r24
	rjmp .L58
.LVL22:
.L38:
.LBB165:
.LBB162:
.LM61:
	mov r24,r25
	subi r24,lo8(-(-97))
	cpi r24,lo8(6)
	brsh .L64
.LM62:
	mov r30,r25
	subi r30,lo8(-(-87))
.LBE162:
.LBE165:
.LM63:
	lds r24,receiveState
	cpi r24,lo8(2)
	brne .L65
.L43:
.LM64:
	cpi r30,lo8(-1)
	brne .+2
	rjmp .L51
.LM65:
	lds r24,debugAddress
	lds r25,(debugAddress)+1
	ldi r31,lo8(0)
.LVL23:
	or r30,r24
	or r31,r25
.LVL24:
	sts (debugAddress)+1,r31
	sts debugAddress,r30
.LM66:
	ldi r24,lo8(3)
	sts receiveState,r24
	rjmp .L58
.LVL25:
.L63:
.LM67:
	cpi r24,lo8(1)
	breq .L66
.LVL26:
.L58:
/* epilogue start */
.LM68:
	pop r31
.LVL27:
	pop r30
.LVL28:
	pop r27
	pop r26
	pop r25
.LVL29:
	pop r24
	pop r0
	out __SREG__,r0
	pop r0
	pop __zero_reg__
	reti
.LVL30:
.L60:
.LM69:
	cpi r24,lo8(5)
	brne .L58
.LM70:
	cpi r30,lo8(-1)
	breq .L57
.LM71:
	lds r24,debugValue
	or r30,r24
	sts debugValue,r30
.LM72:
	lds r30,debugAddress
	lds r31,(debugAddress)+1
.LVL31:
	lds r24,debugValue
	st Z,r24
	rjmp .L57
.LVL32:
.L64:
.LBB166:
.LBB164:
.LM73:
	ldi r30,lo8(-1)
	rjmp .L37
.L66:
.LBE164:
.LBE166:
.LM74:
	cpi r30,lo8(-1)
	brne .+2
	rjmp .L57
.LM75:
	lds r24,debugAddress
	lds r25,(debugAddress)+1
	ldi r31,lo8(0)
.LVL33:
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
.LVL34:
	or r30,r24
	or r31,r25
.LVL35:
	sts (debugAddress)+1,r31
	sts debugAddress,r30
.LM76:
	ldi r24,lo8(2)
	sts receiveState,r24
	rjmp .L58
.LVL36:
.L45:
.LM77:
	cpi r30,lo8(-1)
	brne .+2
	rjmp .L57
.LM78:
	swap r30
	andi r30,lo8(-16)
	sts debugValue,r30
.LM79:
	ldi r24,lo8(5)
	sts receiveState,r24
	rjmp .L58
.L48:
.LM80:
	cpi r25,lo8(76)
	breq .+2
	rjmp .L57
.LM81:
	sts serialLED,__zero_reg__
.LM82:
	ldi r24,lo8(3)
	sts sendState,r24
.LM83:
	call sendNextByte
.LVL37:
	rjmp .L57
.LVL38:
.L51:
.LM84:
	cpi r25,lo8(80)
	brne .+2
	rjmp .L67
.LM85:
	cpi r25,lo8(82)
	breq .+2
	rjmp .L57
.LM86:
	lds r24,debugAddress
	lds r25,(debugAddress)+1
	swap r25
	swap r24
	andi r24,0x0f
	eor r24,r25
	andi r25,0x0f
	eor r24,r25
	sts (debugAddress)+1,r25
	sts debugAddress,r24
.LM87:
	lds r30,debugAddress
	lds r31,(debugAddress)+1
.LVL39:
	subi r30,lo8(-(switchBuffer))
	sbci r31,hi8(-(switchBuffer))
	ld r24,Z
	tst r24
	brne .+2
	rjmp .L57
.LM88:
	lds r30,debugAddress
	lds r31,(debugAddress)+1
	subi r30,lo8(-(switchBuffer))
	sbci r31,hi8(-(switchBuffer))
	st Z,__zero_reg__
.LBB167:
.LM89:
.LVL40:
.LBE167:
	dec r13
.LVL41:
.LM90:
/* #APP */
 ;  242 "v2c.c" 1
	andi r17, 0x7f
 ;  0 "" 2
.LM91:
/* #NOAPP */
	lds r24,debugAddress
	lds r25,(debugAddress)+1
	mov r12,r24
	rjmp .L57
.LVL42:
.L62:
.LM92:
	ldi r24,lo8(4)
	sts receiveState,r24
	rjmp .L58
.L61:
.LM93:
	lds r30,debugAddress
	lds r31,(debugAddress)+1
.LVL43:
	ld r24,Z
.LVL44:
	sts debugValue,r24
.LM94:
	sts receiveState,__zero_reg__
.LM95:
	ldi r24,lo8(1)
.LVL45:
	sts sendState,r24
.LM96:
	call sendNextByte
.LVL46:
	rjmp .L58
.LVL47:
.L67:
.LM97:
	lds r24,debugAddress
	lds r25,(debugAddress)+1
	swap r25
	swap r24
	andi r24,0x0f
	eor r24,r25
	andi r25,0x0f
	eor r24,r25
.LVL48:
	sts (debugAddress)+1,r25
	sts debugAddress,r24
.LM98:
	lds r30,debugAddress
	lds r31,(debugAddress)+1
	subi r30,lo8(-(switchBuffer))
	sbci r31,hi8(-(switchBuffer))
	ld r24,Z
	tst r24
	breq .+2
	rjmp .L57
.LM99:
	lds r30,debugAddress
	lds r31,(debugAddress)+1
	subi r30,lo8(-(switchBuffer))
	sbci r31,hi8(-(switchBuffer))
	ldi r24,lo8(1)
	st Z,r24
.LBB168:
.LM100:
.LVL49:
.LBE168:
	inc r13
.LVL50:
.LM101:
/* #APP */
 ;  232 "v2c.c" 1
	ori r17, 0x80
 ;  0 "" 2
.LM102:
/* #NOAPP */
	lds r24,debugAddress
	lds r25,(debugAddress)+1
	mov r12,r24
	rjmp .L57
.LFE13:
	.size	__vector_18, .-__vector_18
.global	__vector_19
	.type	__vector_19, @function
__vector_19:
.LFB14:
.LM103:
	push __zero_reg__
	push r0
	in r0,__SREG__
	push r0
	clr __zero_reg__
	push r24
	push r25
	push r26
	push r27
	push r30
	push r31
/* prologue: Signal */
/* frame size = 0 */
.LM104:
	lds r24,flags2
	ori r24,lo8(4)
	sts flags2,r24
.LM105:
	call sendNextByte
/* epilogue start */
.LM106:
	pop r31
	pop r30
	pop r27
	pop r26
	pop r25
	pop r24
	pop r0
	out __SREG__,r0
	pop r0
	pop __zero_reg__
	reti
.LFE14:
	.size	__vector_19, .-__vector_19
.global	copySine
	.type	copySine, @function
copySine:
.LFB15:
.LM107:
/* prologue: function */
/* frame size = 0 */
.LM108:
	ldi r24,lo8(0)
	ldi r25,hi8(0)
.LVL51:
.L71:
.LBB169:
.LBB170:
.LM109:
	movw r30,r24
	subi r30,lo8(-(sineTable))
	sbci r31,hi8(-(sineTable))
/* #APP */
 ;  290 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL52:
/* #NOAPP */
.LBE170:
	movw r26,r24
	subi r26,lo8(-(waveform))
	sbci r27,hi8(-(waveform))
	st X,r30
.LM110:
	adiw r24,1
	ldi r30,hi8(256)
	cpi r24,lo8(256)
	cpc r25,r30
.LVL53:
	brne .L71
/* epilogue start */
.LBE169:
.LM111:
	ret
.LFE15:
	.size	copySine, .-copySine
.global	drawMenu
	.type	drawMenu, @function
drawMenu:
.LFB16:
.LM112:
.LVL54:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,10
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 10 */
	std Y+6,r25
	std Y+5,r24
.LM113:
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL55:
.L76:
.LBB171:
.LBB172:
.LBB173:
.LM114:
	ldd r30,Y+5
	ldd r31,Y+6
	ldd r24,Y+3
	ldd r25,Y+4
	add r30,r24
	adc r31,r25
/* #APP */
 ;  296 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL56:
/* #NOAPP */
	lsl r24
	rol r25
	lsl r24
	rol r25
	lsl r24
	rol r25
	std Y+2,r25
	std Y+1,r24
	mov r26,r30
.LVL57:
	ldi r27,lo8(0)
	std Y+8,r27
	std Y+7,r26
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.LVL58:
.L75:
.LBE173:
.LBB174:
.LM115:
	ldd r30,Y+1
	ldd r31,Y+2
.LVL59:
	or r30,r26
	or r31,r27
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ldd r24,Y+7
	ldd r25,Y+8
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	st Z,r24
.LM116:
	adiw r26,1
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L75
.LBE174:
.LBE172:
.LM117:
	ldd r24,Y+3
	ldd r25,Y+4
	adiw r24,1
	std Y+4,r25
	std Y+3,r24
.LVL60:
	sbiw r24,32
	brne .L76
/* epilogue start */
.LBE171:
.LM118:
	adiw r28,10
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE16:
	.size	drawMenu, .-drawMenu
.global	highlight
	.type	highlight, @function
highlight:
.LFB17:
.LM119:
.LVL61:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM120:
	ldi r25,lo8(0)
	movw r30,r24
.LVL62:
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
	andi r30,lo8(-64)
	lsl r24
	rol r25
	lsl r24
	rol r25
	andi r24,lo8(12)
	or r30,r24
	ldi r28,lo8(0)
	ldi r29,hi8(0)
.LVL63:
.L81:
.LBB175:
.LM121:
	movw r24,r28
	swap r24
	andi r24,lo8(-16)
	mov r31,r24
	or r31,r30
	ldi r25,lo8(0)
.L83:
.LBB176:
.LBB177:
.LM122:
	mov r24,r25
	or r24,r31
	mov r26,r24
	ldi r27,lo8(0)
	subi r26,lo8(-(frameBuffer))
	sbci r27,hi8(-(frameBuffer))
	ld r24,X
	tst r24
	breq .L82
.LM123:
	ldi r24,lo8(15)
	st X,r24
.L82:
.LBE177:
.LM124:
	subi r25,lo8(-(1))
	cpi r25,lo8(4)
	brne .L83
	adiw r28,1
.LBE176:
.LM125:
	cpi r28,4
	cpc r29,__zero_reg__
	brne .L81
/* epilogue start */
.LBE175:
.LM126:
	pop r29
	pop r28
	ret
.LFE17:
	.size	highlight, .-highlight
.global	coarseWaveformValue
	.type	coarseWaveformValue, @function
coarseWaveformValue:
.LFB18:
.LM127:
.LVL64:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM128:
	mov r27,r24
	swap r27
.LVL65:
	andi r27,lo8(-16)
	ldi r28,lo8(2048)
	ldi r29,hi8(2048)
.LVL66:
	ldi r26,lo8(0)
.LVL67:
.L88:
.LBB178:
.LM129:
	mov r30,r26
	or r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r24,Z
	clr r25
	sbrc r24,7
	com r25
	add r28,r24
	adc r29,r25
.LM130:
	subi r26,lo8(-(1))
	cpi r26,lo8(16)
	brne .L88
	mov r24,r29
	clr r25
	sbrc r24,7
	dec r25
.LVL68:
.LBE178:
.LM131:
	andi r24,lo8(15)
/* epilogue start */
	pop r29
	pop r28
.LVL69:
	ret
.LFE18:
	.size	coarseWaveformValue, .-coarseWaveformValue
.global	drawEscapeMenu
	.type	drawEscapeMenu, @function
drawEscapeMenu:
.LFB19:
.LM132:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,8
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 8 */
.LM133:
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL70:
.L93:
.LBB179:
.LBB180:
.LBB181:
.LBB182:
.LBB183:
.LM134:
	ldd r30,Y+3
	ldd r31,Y+4
	subi r30,lo8(-(escapeMenu))
	sbci r31,hi8(-(escapeMenu))
/* #APP */
 ;  296 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL71:
/* #NOAPP */
	ldd r24,Y+3
	ldd r25,Y+4
	lsl r24
	rol r25
	lsl r24
	rol r25
	lsl r24
	rol r25
	std Y+2,r25
	std Y+1,r24
	mov r26,r30
.LVL72:
	ldi r27,lo8(0)
	std Y+6,r27
	std Y+5,r26
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.LVL73:
.L92:
.LBE183:
.LBB184:
.LM135:
	ldd r30,Y+1
	ldd r31,Y+2
.LVL74:
	or r30,r26
	or r31,r27
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ldd r24,Y+5
	ldd r25,Y+6
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	st Z,r24
.LM136:
	adiw r26,1
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L92
.LBE184:
.LBE182:
.LM137:
	ldd r24,Y+3
	ldd r25,Y+4
	adiw r24,1
	std Y+4,r25
	std Y+3,r24
.LVL75:
	sbiw r24,32
	brne .L93
.LBE181:
.LBE180:
.LBE179:
.LM138:
	lds r24,waveformPreset
	call highlight
.LVL76:
.LM139:
	lds r24,editor
	subi r24,lo8(-(5))
	call highlight
.LBB185:
.LM140:
.LVL77:
.LBE185:
	sbrc r17,1
	rjmp .L100
.L94:
.LBB186:
.LM141:
.LVL78:
.LBE186:
	sbrc r17,5
	rjmp .L101
.L95:
.LM142:
	lds r24,flags2
	sbrs r24,1
	rjmp .L97
.LM143:
	ldi r24,lo8(14)
	call highlight
.LVL79:
.L97:
/* epilogue start */
.LM144:
	adiw r28,8
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.L101:
.LM145:
	ldi r24,lo8(13)
	call highlight
.LVL80:
	rjmp .L95
.LVL81:
.L100:
.LM146:
	ldi r24,lo8(12)
	call highlight
.LVL82:
	rjmp .L94
.LFE19:
	.size	drawEscapeMenu, .-drawEscapeMenu
.global	drawEEPROMScreen
	.type	drawEEPROMScreen, @function
drawEEPROMScreen:
.LFB20:
.LM147:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,9
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 9 */
.LM148:
	ldi r24,lo8(frameBuffer)
	ldi r25,hi8(frameBuffer)
	std Y+2,r25
	std Y+1,r24
	std Y+5,__zero_reg__
	std Y+4,__zero_reg__
.LVL83:
.L103:
.LBB187:
.LM149:
	ldd r30,Y+4
	ldd r31,Y+5
	lsl r30
	rol r31
	lsl r30
	rol r31
	std Y+7,r31
	std Y+6,r30
	std Y+3,__zero_reg__
.LVL84:
	std Y+9,__zero_reg__
	std Y+8,__zero_reg__
.LVL85:
	ldi r30,lo8(0)
	ldi r31,hi8(0)
.LVL86:
.L114:
.LBB188:
.LBB189:
.LBB190:
.LBB191:
.LBB192:
.LM150:
	sbic 63-32,1
	rjmp .L114
.LM151:
	ldd r24,Y+6
	ldd r25,Y+7
.LVL87:
	or r24,r30
	or r25,r31
	out (65)+1-32,r25
	out 65-32,r24
.LM152:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL88:
/* #NOAPP */
.LBE192:
.LBE191:
.LM153:
	tst r24
	breq .L105
.LM154:
	ldd r26,Y+8
	ldd r27,Y+9
	add r26,r24
	adc r27,__zero_reg__
	std Y+9,r27
	std Y+8,r26
.LVL89:
	ldi r27,lo8(1)
	std Y+3,r27
.LVL90:
.L105:
	adiw r30,1
.LBE190:
.LM155:
	cpi r30,4
	cpc r31,__zero_reg__
	brne .L114
.LBE189:
.LM156:
	ldd r30,Y+3
	tst r30
	breq .L106
.LM157:
	ldd r24,Y+8
	ldd r25,Y+9
.LVL91:
	clr __tmp_reg__
	lsl r24
	rol r25
	rol __tmp_reg__
	lsl r24
	rol r25
	rol __tmp_reg__
	mov r24,r25
	mov r25,__tmp_reg__
.LVL92:
.LM158:
	sbiw r24,0
	brne .L108
	ldi r24,lo8(1)
.LVL93:
.L108:
.LM159:
	ldd r26,Y+1
	ldd r27,Y+2
	st X,r24
.LVL94:
.L109:
.LBE188:
.LM160:
	ldd r24,Y+4
	ldd r25,Y+5
.LVL95:
	adiw r24,1
	std Y+5,r25
	std Y+4,r24
.LVL96:
	ldd r26,Y+1
	ldd r27,Y+2
	adiw r26,1
	std Y+2,r27
	std Y+1,r26
	subi r24,lo8(256)
	sbci r25,hi8(256)
	breq .+2
	rjmp .L103
/* epilogue start */
.LBE187:
.LM161:
	adiw r28,9
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LVL97:
.L106:
.LBB194:
.LBB193:
.LM162:
	ldd r30,Y+1
	ldd r31,Y+2
	st Z,__zero_reg__
	rjmp .L109
.LBE193:
.LBE194:
.LFE20:
	.size	drawEEPROMScreen, .-drawEEPROMScreen
.global	startPatternEditor
	.type	startPatternEditor, @function
startPatternEditor:
.LFB21:
.LM163:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,8
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 8 */
.LM164:
	lds r24,flags2
	sbrs r24,1
	rjmp .L116
.LM165:
/* #APP */
 ;  361 "v2c.c" 1
	andi r17, 0xbf
 ;  0 "" 2
/* #NOAPP */
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL98:
.L118:
.LBB195:
.LBB196:
.LBB197:
.LBB198:
.LBB199:
.LM166:
	ldd r30,Y+3
	ldd r31,Y+4
	subi r30,lo8(-(microtoneScreen))
	sbci r31,hi8(-(microtoneScreen))
/* #APP */
 ;  296 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL99:
/* #NOAPP */
	ldd r24,Y+3
	ldd r25,Y+4
	lsl r24
	rol r25
	lsl r24
	rol r25
	lsl r24
	rol r25
	std Y+2,r25
	std Y+1,r24
	mov r26,r30
.LVL100:
	ldi r27,lo8(0)
	std Y+6,r27
	std Y+5,r26
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.LVL101:
.L117:
.LBE199:
.LBB200:
.LM167:
	ldd r30,Y+1
	ldd r31,Y+2
.LVL102:
	or r30,r26
	or r31,r27
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ldd r24,Y+5
	ldd r25,Y+6
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	st Z,r24
.LM168:
	adiw r26,1
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L117
.LBE200:
.LBE198:
.LM169:
	ldd r24,Y+3
	ldd r25,Y+4
	adiw r24,1
	std Y+4,r25
	std Y+3,r24
.LVL103:
	sbiw r24,32
	brne .L118
.L120:
/* epilogue start */
.LBE197:
.LBE196:
.LBE195:
.LM170:
	adiw r28,8
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LVL104:
.L116:
.LM171:
/* #APP */
 ;  365 "v2c.c" 1
	ori r17, 0x40
 ;  0 "" 2
/* #NOAPP */
	rjmp .L120
.LFE21:
	.size	startPatternEditor, .-startPatternEditor
.global	startCoarseEditor
	.type	startCoarseEditor, @function
startCoarseEditor:
.LFB22:
.LM172:
	push r29
	push r28
	rcall .
	rcall .
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 4 */
.LM173:
	ldi r30,lo8(frameBuffer)
	ldi r31,hi8(frameBuffer)
.L124:
.LM174:
	st Z+,__zero_reg__
.LM175:
	ldi r24,hi8(frameBuffer+256)
	cpi r30,lo8(frameBuffer+256)
	cpc r31,r24
	brne .L124
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL105:
.LVL106:
.L126:
.LBB201:
.LBB202:
.LM176:
	ldd r25,Y+3
	swap r25
	andi r25,lo8(-16)
	std Y+2,r25
.LVL107:
	std Y+1,__zero_reg__
.LVL108:
	ldi r26,lo8(2048)
	ldi r27,hi8(2048)
.LVL109:
.L125:
.LBB203:
.LM177:
	ldd r25,Y+1
	ldd r30,Y+2
	or r25,r30
	mov r30,r25
	ldi r31,lo8(0)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r24,Z
	clr r25
	sbrc r24,7
	com r25
	add r26,r24
	adc r27,r25
.LM178:
	ldd r31,Y+1
	subi r31,lo8(-(1))
	std Y+1,r31
.LVL110:
	cpi r31,lo8(16)
	brne .L125
.LBE203:
.LBE202:
.LBE201:
.LM179:
	mov r24,r27
	clr r25
	sbrc r24,7
	dec r25
	andi r24,lo8(15)
	mov r30,r24
	ldi r31,lo8(0)
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
.LVL111:
	ldd r24,Y+3
	ldd r25,Y+4
	or r30,r24
	or r31,r25
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ldi r25,lo8(1)
	st Z,r25
.LM180:
	ldd r30,Y+3
	ldd r31,Y+4
	adiw r30,1
	std Y+4,r31
	std Y+3,r30
.LVL112:
	sbiw r30,16
	brne .L126
/* epilogue start */
.LM181:
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop r28
	pop r29
	ret
.LFE22:
	.size	startCoarseEditor, .-startCoarseEditor
.global	startFineEditor
	.type	startFineEditor, @function
startFineEditor:
.LFB23:
.LM182:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM183:
	ldi r28,lo8(0)
	ldi r29,hi8(0)
.LVL113:
.L132:
.LBB204:
.LM184:
	movw r26,r28
	subi r26,lo8(-(frameBuffer))
	sbci r27,hi8(-(frameBuffer))
	movw r30,r28
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r24,Z
	clr r25
	sbrc r24,7
	com r25
	subi r24,lo8(-(128))
	sbci r25,hi8(-(128))
	asr r25
	ror r24
	asr r25
	ror r24
	asr r25
	ror r24
	asr r25
	ror r24
	st X,r24
.LM185:
	adiw r28,1
	ldi r24,hi8(256)
	cpi r28,lo8(256)
	cpc r29,r24
	brne .L132
/* epilogue start */
.LBE204:
.LM186:
	pop r29
	pop r28
.LVL114:
	ret
.LFE23:
	.size	startFineEditor, .-startFineEditor
.global	startTuningEditor
	.type	startTuningEditor, @function
startTuningEditor:
.LFB24:
.LM187:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,10
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 10 */
.LM188:
	ldi r24,lo8(frequencies)
	ldi r25,hi8(frequencies)
	std Y+4,r25
	std Y+3,r24
	std Y+6,__zero_reg__
	std Y+5,__zero_reg__
.L136:
.LM189:
	ldd r30,Y+5
	ldd r31,Y+6
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
	std Y+2,r31
	std Y+1,r30
	ldd r30,Y+3
	ldd r31,Y+4
	ld __tmp_reg__,Z+
	ld r31,Z
	mov r30,__tmp_reg__
	std Y+8,r31
	std Y+7,r30
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L138:
.LM190:
	ldd r30,Y+1
	ldd r31,Y+2
	or r30,r26
	or r31,r27
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	std Y+10,r31
	std Y+9,r30
	st Z,__zero_reg__
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	mov r0,r26
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	ldd r30,Y+7
	ldd r31,Y+8
	and r24,r30
	and r25,r31
	or r24,r25
	breq .L137
	ldi r24,lo8(1)
	ldd r30,Y+9
	ldd r31,Y+10
	st Z,r24
.L137:
	adiw r26,1
.LM191:
	cpi r26,16
	cpc r27,__zero_reg__
	brne .L138
	ldd r30,Y+5
	ldd r31,Y+6
	adiw r30,1
	std Y+6,r31
	std Y+5,r30
	ldd r24,Y+3
	ldd r25,Y+4
	adiw r24,2
	std Y+4,r25
	std Y+3,r24
.LM192:
	sbiw r30,16
	brne .L136
/* epilogue start */
.LM193:
	adiw r28,10
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE24:
	.size	startTuningEditor, .-startTuningEditor
.global	startMiscellaneousEditor
	.type	startMiscellaneousEditor, @function
startMiscellaneousEditor:
.LFB25:
.LM194:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,12
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 12 */
.LM195:
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL115:
.L144:
.LBB205:
.LBB206:
.LBB207:
.LBB208:
.LBB209:
.LM196:
	ldd r30,Y+3
	ldd r31,Y+4
	subi r30,lo8(-(miscellaneousMenu))
	sbci r31,hi8(-(miscellaneousMenu))
/* #APP */
 ;  296 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL116:
/* #NOAPP */
	ldd r24,Y+3
	ldd r25,Y+4
	lsl r24
	rol r25
	lsl r24
	rol r25
	lsl r24
	rol r25
	std Y+2,r25
	std Y+1,r24
	mov r26,r30
.LVL117:
	ldi r27,lo8(0)
	std Y+6,r27
	std Y+5,r26
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.LVL118:
.L143:
.LBE209:
.LBB210:
.LM197:
	ldd r30,Y+1
	ldd r31,Y+2
.LVL119:
	or r30,r26
	or r31,r27
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ldd r24,Y+5
	ldd r25,Y+6
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	st Z,r24
.LM198:
	adiw r26,1
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L143
.LBE210:
.LBE208:
.LM199:
	ldd r24,Y+3
	ldd r25,Y+4
	adiw r24,1
	std Y+4,r25
	std Y+3,r24
.LVL120:
	sbiw r24,32
	brne .L144
.LBE207:
.LBE206:
.LBE205:
.LM200:
	lds r26,framesPerBeatOverride
	lds r27,(framesPerBeatOverride)+1
.LVL121:
	std Y+6,r27
	std Y+5,r26
	std Y+12,__zero_reg__
	std Y+11,__zero_reg__
.L146:
	ldd r30,Y+11
	ldd r31,Y+12
	andi r30,lo8(3)
	andi r31,hi8(3)
	ldd r24,Y+11
	ldd r25,Y+12
	lsl r24
	rol r25
	lsl r24
	rol r25
	andi r24,lo8(48)
	andi r25,hi8(48)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	add r30,r24
	adc r31,r25
	std Z+17,__zero_reg__
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	ldd r0,Y+11
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	ldd r26,Y+5
	ldd r27,Y+6
	and r24,r26
	and r25,r27
	or r24,r25
	breq .L145
	ldi r27,lo8(1)
	std Z+17,r27
.L145:
	ldd r30,Y+11
	ldd r31,Y+12
	adiw r30,1
	std Y+12,r31
	std Y+11,r30
.LM201:
	sbiw r30,16
	brne .L146
.LM202:
	lds r24,decayConstantOverride
	mov r26,r24
	ldi r27,lo8(0)
	std Y+6,r27
	std Y+5,r26
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L147:
	movw r30,r26
	andi r30,lo8(3)
	andi r31,hi8(3)
	movw r24,r26
	lsl r24
	rol r25
	lsl r24
	rol r25
	andi r24,lo8(48)
	andi r25,hi8(48)
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(frameBuffer+129))
	sbci r31,hi8(-(frameBuffer+129))
	ldd r24,Y+5
	ldd r25,Y+6
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	st Z,r24
	adiw r26,1
.LM203:
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L147
.LM204:
	mov r30,r14
	ldi r31,lo8(0)
	std Y+6,r31
	std Y+5,r30
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L148:
	movw r30,r26
	andi r30,lo8(1)
	andi r31,hi8(1)
	movw r24,r26
	lsl r24
	rol r25
	andi r24,lo8(48)
	andi r25,hi8(48)
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(frameBuffer+209))
	sbci r31,hi8(-(frameBuffer+209))
	ldd r24,Y+5
	ldd r25,Y+6
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	st Z,r24
	adiw r26,1
.LM205:
	cpi r26,4
	cpc r27,__zero_reg__
	brne .L148
.LM206:
	lds r24,patternsPerLoop
	mov r26,r24
	ldi r27,lo8(0)
	std Y+6,r27
	std Y+5,r26
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L149:
	movw r30,r26
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
	andi r30,lo8(112)
	andi r31,hi8(112)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ldd r24,Y+5
	ldd r25,Y+6
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
	andi r24,lo8(1)
	std Z+30,r24
	adiw r26,1
.LM207:
	cpi r26,5
	cpc r27,__zero_reg__
	brne .L149
.LM208:
	sts frameBuffer+158,__zero_reg__
	lds r25,flags2
	sbrs r25,3
	rjmp .L150
	ldi r24,lo8(1)
	sts frameBuffer+158,r24
.L150:
.LM209:
	sts frameBuffer+222,__zero_reg__
	sbrs r25,4
	rjmp .L152
	ldi r24,lo8(1)
	sts frameBuffer+222,r24
.L152:
/* epilogue start */
.LM210:
	adiw r28,12
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE25:
	.size	startMiscellaneousEditor, .-startMiscellaneousEditor
.global	startEditor
	.type	startEditor, @function
startEditor:
.LFB26:
.LM211:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,10
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 10 */
.LM212:
	sts mode,__zero_reg__
.LM213:
	lds r24,editor
	cpi r24,lo8(2)
	breq .L163
	cpi r24,lo8(3)
	brlo .L174
	cpi r24,lo8(3)
	breq .L164
	cpi r24,lo8(4)
	breq .L175
.LVL122:
.L171:
/* epilogue start */
.LM214:
	adiw r28,10
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LVL123:
.L174:
.LM215:
	tst r24
	breq .L161
	cpi r24,lo8(1)
	brne .L171
.LM216:
	call startCoarseEditor
	rjmp .L171
.L161:
.LM217:
	call startPatternEditor
	rjmp .L171
.L175:
.LM218:
	call startMiscellaneousEditor
	rjmp .L171
.L163:
.LM219:
	std Y+8,__zero_reg__
	std Y+7,__zero_reg__
.LVL124:
.L168:
.LBB211:
.LBB212:
.LBB213:
.LM220:
	ldd r26,Y+7
	ldd r27,Y+8
	subi r26,lo8(-(frameBuffer))
	sbci r27,hi8(-(frameBuffer))
	ldd r30,Y+7
	ldd r31,Y+8
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r24,Z
	clr r25
	sbrc r24,7
	com r25
	subi r24,lo8(-(128))
	sbci r25,hi8(-(128))
	asr r25
	ror r24
	asr r25
	ror r24
	asr r25
	ror r24
	asr r25
	ror r24
	st X,r24
.LM221:
	ldd r30,Y+7
	ldd r31,Y+8
	adiw r30,1
	std Y+8,r31
	std Y+7,r30
.LVL125:
	subi r30,lo8(256)
	sbci r31,hi8(256)
	brne .L168
	rjmp .L171
.LVL126:
.L164:
.LBE213:
.LBE212:
.LBE211:
.LM222:
	ldi r24,lo8(frequencies)
	ldi r25,hi8(frequencies)
	std Y+4,r25
	std Y+3,r24
	std Y+6,__zero_reg__
	std Y+5,__zero_reg__
.L167:
.LBB214:
.LBB215:
.LM223:
	ldd r30,Y+5
	ldd r31,Y+6
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
	std Y+2,r31
	std Y+1,r30
	ldd r30,Y+3
	ldd r31,Y+4
	ld __tmp_reg__,Z+
	ld r31,Z
	mov r30,__tmp_reg__
	std Y+8,r31
	std Y+7,r30
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L170:
.LM224:
	ldd r30,Y+1
	ldd r31,Y+2
	or r30,r26
	or r31,r27
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	std Y+10,r31
	std Y+9,r30
	st Z,__zero_reg__
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	mov r0,r26
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	ldd r30,Y+7
	ldd r31,Y+8
	and r24,r30
	and r25,r31
	or r24,r25
	breq .L169
	ldi r24,lo8(1)
	ldd r30,Y+9
	ldd r31,Y+10
	st Z,r24
.L169:
	adiw r26,1
.LM225:
	cpi r26,16
	cpc r27,__zero_reg__
	brne .L170
	ldd r30,Y+5
	ldd r31,Y+6
	adiw r30,1
	std Y+6,r31
	std Y+5,r30
	ldd r24,Y+3
	ldd r25,Y+4
	adiw r24,2
	std Y+4,r25
	std Y+3,r24
.LM226:
	sbiw r30,16
	brne .L167
	rjmp .L171
.LBE215:
.LBE214:
.LFE26:
	.size	startEditor, .-startEditor
.global	microtoneModeTouch
	.type	microtoneModeTouch, @function
microtoneModeTouch:
.LFB27:
.LM227:
.LVL127:
	push r29
	push r28
	rcall .
	rcall .
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 4 */
	std Y+2,r24
.LM228:
	ldi r25,lo8(-1)
.LVL128:
	std Y+1,__zero_reg__
.LVL129:
	ldi r26,lo8(0)
	ldi r27,hi8(0)
	rjmp .L181
.LVL130:
.L183:
.LBB216:
.LM229:
	mov r24,r25
	adiw r26,1
.LM230:
	cpi r26,16
	cpc r27,__zero_reg__
	breq .L180
.LVL131:
.L184:
	mov r25,r24
.LVL132:
.L181:
.LBE216:
.LM231:
	std Y+4,r27
	std Y+3,r26
.LBB217:
.LM232:
	movw r30,r26
	subi r30,lo8(-(microtoneKeys))
	sbci r31,hi8(-(microtoneKeys))
	ld r24,Z
	ldd r30,Y+2
	cp r24,r30
	breq .L177
.LM233:
	movw r30,r26
	subi r30,lo8(-(volumes))
	sbci r31,hi8(-(volumes))
	ld r24,Z
.LVL133:
	cp r24,r25
	brsh .L183
.LVL134:
	std Y+1,r26
.LVL135:
	adiw r26,1
.LM234:
	cpi r26,16
	cpc r27,__zero_reg__
	brne .L184
.LVL136:
.L180:
	ldd r25,Y+1
	mov r24,r25
.LVL137:
	ldi r25,lo8(0)
	std Y+4,r25
	std Y+3,r24
.LVL138:
.L177:
.LBE217:
.LM235:
	ldd r30,Y+3
	ldd r31,Y+4
	subi r30,lo8(-(microtoneKeys))
	sbci r31,hi8(-(microtoneKeys))
	ldd r26,Y+2
	st Z,r26
.LM236:
	ldd r30,Y+3
	ldd r31,Y+4
	subi r30,lo8(-(volumes))
	sbci r31,hi8(-(volumes))
	ldi r24,lo8(-1)
	st Z,r24
/* epilogue start */
.LM237:
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop r28
	pop r29
	ret
.LFE27:
	.size	microtoneModeTouch, .-microtoneModeTouch
.global	patternEditorTouch
	.type	patternEditorTouch, @function
patternEditorTouch:
.LFB28:
.LM238:
.LVL139:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,7
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 7 */
	std Y+3,r24
.LM239:
	lds r24,flags2
.LVL140:
	sbrs r24,1
	rjmp .L186
	ldi r25,lo8(-1)
.LVL141:
	std Y+1,__zero_reg__
.LVL142:
	ldi r26,lo8(0)
	ldi r27,hi8(0)
	rjmp .L190
.LVL143:
.L195:
.LBB218:
.LBB219:
.LBB220:
.LM240:
	mov r25,r30
	ldd r24,Y+2
	std Y+1,r24
.LVL144:
.L190:
.LBE220:
.LBE219:
.LBE218:
.LM241:
	std Y+2,r26
.LVL145:
	std Y+5,r27
	std Y+4,r26
.LBB223:
.LBB222:
.LBB221:
.LM242:
	movw r30,r26
	subi r30,lo8(-(microtoneKeys))
	sbci r31,hi8(-(microtoneKeys))
	ld r24,Z
	ldd r30,Y+3
	cp r30,r24
	breq .L187
.LM243:
	movw r30,r26
	subi r30,lo8(-(volumes))
	sbci r31,hi8(-(volumes))
	ld r30,Z
.LVL146:
	cp r30,r25
	brlo .L188
.LVL147:
	ldd r31,Y+1
	std Y+2,r31
.LVL148:
	mov r30,r25
.LVL149:
.L188:
	adiw r26,1
.LM244:
	cpi r26,16
	cpc r27,__zero_reg__
	brne .L195
	ldd r27,Y+2
	mov r26,r27
	ldi r27,lo8(0)
	std Y+5,r27
	std Y+4,r26
.LVL150:
.L187:
.LBE221:
.LM245:
	ldd r30,Y+4
	ldd r31,Y+5
	subi r30,lo8(-(microtoneKeys))
	sbci r31,hi8(-(microtoneKeys))
	ldd r24,Y+3
	st Z,r24
.LM246:
	ldd r26,Y+4
	ldd r27,Y+5
	subi r26,lo8(-(volumes))
	sbci r27,hi8(-(volumes))
	ldi r24,lo8(-1)
	st X,r24
.LVL151:
.L194:
/* epilogue start */
.LBE222:
.LBE223:
.LM247:
	adiw r28,7
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LVL152:
.L186:
.LBB224:
.LM248:
.LVL153:
.LBE224:
	mov r30,r10
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	ldd r27,Y+3
	mov r26,r27
	ldi r27,lo8(0)
	std Y+7,r27
	std Y+6,r26
	mov r31,r30
	clr r30
	add r30,r26
	adc r31,r27
	subi r30,lo8(-(lightBuffers))
	sbci r31,hi8(-(lightBuffers))
	ld r30,Z
.LVL154:
.LM249:
	sbrc r24,0
	rjmp .L192
.LM250:
	mov r25,r30
	andi r25,lo8(1)
	swap r25
	lsl r25
	andi r25,lo8(-32)
	andi r24,lo8(-33)
	or r24,r25
.LM251:
	ori r24,lo8(1)
	sts flags2,r24
.L192:
.LM252:
	tst r30
	brne .L193
.LM253:
	sbrc r24,5
	rjmp .L194
.LBB225:
.LM254:
.LVL155:
.LBE225:
	mov r30,r10
.LVL156:
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	mov r31,r30
	clr r30
	ldd r24,Y+6
	ldd r25,Y+7
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(lightBuffers))
	sbci r31,hi8(-(lightBuffers))
	ldi r24,lo8(1)
	st Z,r24
.LM255:
	subi r16,lo8(-(1))
.LBB226:
.LM256:
.LVL157:
.LBE226:
	mov r30,r15
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	mov r31,r30
	clr r30
	ldd r26,Y+6
	ldd r27,Y+7
	add r30,r26
	adc r31,r27
	subi r30,lo8(-(waterBuffers))
	sbci r31,hi8(-(waterBuffers))
	ldi r24,lo8(16)
	st Z,r24
	rjmp .L194
.LVL158:
.L193:
.LM257:
	sbrs r24,5
	rjmp .L194
.LBB227:
.LM258:
.LVL159:
.LBE227:
	mov r30,r10
.LVL160:
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	mov r31,r30
	clr r30
	ldd r24,Y+6
	ldd r25,Y+7
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(lightBuffers))
	sbci r31,hi8(-(lightBuffers))
	st Z,__zero_reg__
.LM259:
	subi r16,lo8(-(-1))
.LBB228:
.LM260:
.LVL161:
.LBE228:
	mov r30,r15
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	mov r31,r30
	clr r30
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(waterBuffers))
	sbci r31,hi8(-(waterBuffers))
	ldi r24,lo8(16)
	st Z,r24
	rjmp .L194
.LFE28:
	.size	patternEditorTouch, .-patternEditorTouch
.global	coarseEditorTouch
	.type	coarseEditorTouch, @function
coarseEditorTouch:
.LFB29:
.LM261:
.LVL162:
	push r29
	push r28
	rcall .
	push __tmp_reg__
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 3 */
	std Y+1,r24
.LBB229:
.LM262:
	mov r30,r24
	swap r30
	andi r30,lo8(15)
	ldi r31,lo8(0)
	subi r30,lo8(-(speakerPositions))
	sbci r31,hi8(-(speakerPositions))
/* #APP */
 ;  469 "v2c.c" 1
	lpm r26, Z
	
 ;  0 "" 2
.LVL163:
/* #NOAPP */
.LBE229:
	andi r24,lo8(15)
.LVL164:
	mov r30,r24
	ldi r31,lo8(0)
	std Y+3,r31
	std Y+2,r30
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ldi r25,lo8(0)
.LVL165:
.L197:
.LBB230:
.LM263:
	st Z+,r26
.LM264:
	subi r25,lo8(-(1))
	cpi r25,lo8(16)
	brne .L197
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.LVL166:
.L198:
.LBE230:
.LBB231:
.LM265:
	movw r30,r26
	swap r30
	swap r31
	andi r31,0xf0
	eor r31,r30
	andi r30,0xf0
	eor r31,r30
	ldd r24,Y+2
	ldd r25,Y+3
	or r30,r24
	or r31,r25
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	st Z,__zero_reg__
	adiw r26,1
.LM266:
	cpi r26,16
	cpc r27,__zero_reg__
	brne .L198
.LBE231:
.LM267:
	ldd r25,Y+1
.LVL167:
	mov r30,r25
.LVL168:
	ldi r31,lo8(0)
.LVL169:
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
.LVL170:
	ldi r24,lo8(1)
	st Z,r24
/* epilogue start */
.LM268:
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop r28
	pop r29
	ret
.LFE29:
	.size	coarseEditorTouch, .-coarseEditorTouch
.global	fineEditorTouch
	.type	fineEditorTouch, @function
fineEditorTouch:
.LFB30:
.LM269:
.LVL171:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM270:
	mov r26,r24
	ldi r27,lo8(0)
	movw r28,r26
	subi r28,lo8(-(waveform))
	sbci r29,hi8(-(waveform))
	ld r24,Y
.LVL172:
	asr r24
	asr r24
	asr r24
	asr r24
	subi r24,lo8(-(9))
	andi r24,lo8(15)
.LBB232:
.LM271:
	ldi r30,lo8(speakerPositions)
	ldi r31,hi8(speakerPositions)
	add r30,r24
	adc r31,__zero_reg__
/* #APP */
 ;  480 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL173:
/* #NOAPP */
.LBE232:
	st Y,r30
.LM272:
	subi r26,lo8(-(frameBuffer))
	sbci r27,hi8(-(frameBuffer))
	st X,r24
/* epilogue start */
.LM273:
	pop r29
	pop r28
	ret
.LFE30:
	.size	fineEditorTouch, .-fineEditorTouch
.global	tuningEditorTouch
	.type	tuningEditorTouch, @function
tuningEditorTouch:
.LFB31:
.LM274:
.LVL174:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
	mov r26,r24
.LM275:
	mov r28,r24
	ldi r29,lo8(0)
	movw r30,r28
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
.LVL175:
	tst r24
	breq .L205
	st Z,__zero_reg__
.LM276:
	swap r26
	andi r26,lo8(15)
	mov r30,r26
	ldi r31,lo8(0)
	lsl r30
	rol r31
.LVL176:
	subi r30,lo8(-(frequencies))
	sbci r31,hi8(-(frequencies))
	andi r28,lo8(15)
	andi r29,hi8(15)
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r28
	brpl 1b
	com r24
	com r25
	ld r26,Z
	ldd r27,Z+1
	and r24,r26
	and r25,r27
	std Z+1,r25
	st Z,r24
/* epilogue start */
.LM277:
	pop r29
	pop r28
	ret
.LVL177:
.L205:
.LM278:
	ldi r24,lo8(15)
	st Z,r24
.LM279:
	swap r26
	andi r26,lo8(15)
	ldi r27,lo8(0)
	lsl r26
	rol r27
.LVL178:
	subi r26,lo8(-(frequencies))
	sbci r27,hi8(-(frequencies))
	andi r28,lo8(15)
	andi r29,hi8(15)
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r28
	brpl 1b
	ld r30,X+
	ld r31,X
	sbiw r26,1
	or r24,r30
	or r25,r31
	st X+,r24
	st X,r25
.LM280:
	pop r29
	pop r28
	ret
.LFE31:
	.size	tuningEditorTouch, .-tuningEditorTouch
.global	miscellaneousEditorTouch
	.type	miscellaneousEditorTouch, @function
miscellaneousEditorTouch:
.LFB32:
.LM281:
.LVL179:
	push r29
	push r28
	rcall .
	rcall .
	rcall .
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 6 */
	mov r27,r24
.LM282:
	andi r24,lo8(15)
.LVL180:
	std Y+3,r24
.LVL181:
.LM283:
	mov r26,r27
.LVL182:
	swap r26
	andi r26,lo8(15)
.LM284:
	subi r24,lo8(-(-1))
	std Y+4,r24
	cpi r24,lo8(4)
	brlo .+2
	rjmp .L209
.LM285:
	mov r24,r26
	subi r24,lo8(-(-1))
	cpi r24,lo8(4)
	brsh .L210
.LM286:
	mov r24,r26
	ldi r25,lo8(0)
.LVL183:
	sbiw r24,1
.LVL184:
	lsl r24
	rol r25
	lsl r24
	rol r25
.LVL185:
	ldd r25,Y+4
.LVL186:
	or r25,r24
.LM287:
	mov r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
.LVL187:
	tst r24
	brne .+2
	rjmp .L211
	st Z,__zero_reg__
.LM288:
	ldi r30,lo8(1)
	ldi r31,hi8(1)
	rjmp 2f
1:	lsl r30
	rol r31
2:	dec r25
	brpl 1b
	com r30
	com r31
	lds r24,framesPerBeatOverride
	lds r25,(framesPerBeatOverride)+1
	and r24,r30
	and r25,r31
	sts (framesPerBeatOverride)+1,r25
	sts framesPerBeatOverride,r24
.LVL188:
.L210:
.LM289:
	mov r24,r26
	subi r24,lo8(-(-8))
	cpi r24,lo8(2)
	brsh .+2
	rjmp .L222
.LVL189:
.L212:
.LM290:
	ldd r24,Y+3
	cpi r24,lo8(3)
	brsh .L209
	mov r24,r26
	subi r24,lo8(-(-13))
	cpi r24,lo8(2)
	brsh .+2
	rjmp .L223
.LVL190:
.L221:
/* epilogue start */
.LM291:
	adiw r28,6
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LVL191:
.L209:
.LM292:
	ldd r25,Y+3
	cpi r25,lo8(14)
	brne .L221
.LM293:
	mov r30,r26
	subi r30,lo8(-(-1))
	std Y+1,r30
	cpi r30,lo8(5)
	brsh .L216
.LM294:
	mov r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
	tst r24
	brne .+2
	rjmp .L217
	st Z,__zero_reg__
.LM295:
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	ldd r0,Y+1
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	com r24
	lds r30,patternsPerLoop
	and r30,r24
	sts patternsPerLoop,r30
.L216:
.LM296:
	cpi r26,lo8(9)
	brne .+2
	rjmp .L224
.LM297:
	cpi r26,lo8(13)
	brne .L221
.LM298:
	mov r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
	tst r24
	brne .+2
	rjmp .L220
	st Z,__zero_reg__
.LM299:
	lds r24,flags2
	andi r24,lo8(-17)
	sts flags2,r24
	rjmp .L221
.LVL192:
.L222:
.LM300:
	mov r30,r26
	ldi r31,lo8(0)
	sbiw r30,8
	lsl r30
	rol r31
	lsl r30
	rol r31
	ldd r31,Y+4
	or r31,r30
	std Y+2,r31
.LVL193:
.LM301:
	mov r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
	tst r24
	brne .+2
	rjmp .L213
	st Z,__zero_reg__
.LM302:
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	ldd r0,Y+2
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	com r24
	lds r30,decayConstantOverride
	and r30,r24
	sts decayConstantOverride,r30
	rjmp .L212
.LVL194:
.L211:
.LM303:
	ldi r24,lo8(15)
	st Z,r24
.LM304:
	ldi r30,lo8(1)
	ldi r31,hi8(1)
	rjmp 2f
1:	lsl r30
	rol r31
2:	dec r25
	brpl 1b
	lds r24,framesPerBeatOverride
	lds r25,(framesPerBeatOverride)+1
	or r24,r30
	or r25,r31
	sts (framesPerBeatOverride)+1,r25
	sts framesPerBeatOverride,r24
.LM305:
	mov r24,r26
.LVL195:
	subi r24,lo8(-(-8))
	cpi r24,lo8(2)
	brlo .+2
	rjmp .L212
	rjmp .L222
.LVL196:
.L223:
.LM306:
	mov r24,r26
	ldi r25,lo8(0)
.LVL197:
	sbiw r24,13
.LVL198:
	lsl r24
	rol r25
	lsl r24
	rol r25
.LVL199:
	ldd r26,Y+4
.LVL200:
	or r26,r24
.LM307:
	mov r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
.LVL201:
	tst r24
	brne .+2
	rjmp .L215
	st Z,__zero_reg__
.LM308:
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r26
	brpl 1b
	com r24
	and r14,r24
	rjmp .L221
.LVL202:
.L224:
.LM309:
	mov r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(frameBuffer))
	sbci r31,hi8(-(frameBuffer))
	ld r24,Z
	tst r24
	breq .L219
	st Z,__zero_reg__
.LM310:
	lds r24,flags2
	andi r24,lo8(-9)
	sts flags2,r24
	rjmp .L221
.L217:
.LM311:
	ldi r24,lo8(15)
	st Z,r24
.LM312:
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	ldd r0,Y+1
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	lds r30,patternsPerLoop
	or r30,r24
	sts patternsPerLoop,r30
	rjmp .L216
.L213:
.LM313:
	ldi r24,lo8(15)
	st Z,r24
.LM314:
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	ldd r0,Y+2
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r0
	brpl 1b
	lds r30,decayConstantOverride
	or r30,r24
	sts decayConstantOverride,r30
	rjmp .L212
.L220:
.LM315:
	ldi r24,lo8(15)
	st Z,r24
.LM316:
	lds r24,flags2
	ori r24,lo8(16)
	sts flags2,r24
	rjmp .L221
.L219:
.LM317:
	ldi r24,lo8(15)
	st Z,r24
.LM318:
	lds r24,flags2
	ori r24,lo8(8)
	sts flags2,r24
	rjmp .L221
.LVL203:
.L215:
.LM319:
	ldi r24,lo8(15)
	st Z,r24
.LM320:
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	rjmp 2f
1:	lsl r24
	rol r25
2:	dec r26
	brpl 1b
	or r14,r24
	rjmp .L221
.LFE32:
	.size	miscellaneousEditorTouch, .-miscellaneousEditorTouch
.global	editorTouch
	.type	editorTouch, @function
editorTouch:
.LFB33:
.LM321:
.LVL204:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM322:
	lds r25,editor
	cpi r25,lo8(2)
	breq .L229
	cpi r25,lo8(3)
	brlo .L234
	cpi r25,lo8(3)
	breq .L230
	cpi r25,lo8(4)
	breq .L235
.L233:
/* epilogue start */
.LM323:
	pop r29
	pop r28
	ret
.L234:
.LM324:
	tst r25
	breq .L227
	cpi r25,lo8(1)
	brne .L233
.LM325:
	call coarseEditorTouch
.LVL205:
.LM326:
	pop r29
	pop r28
	ret
.LVL206:
.L227:
.LM327:
	call patternEditorTouch
.LVL207:
.LM328:
	pop r29
	pop r28
	ret
.LVL208:
.L235:
.LM329:
	call miscellaneousEditorTouch
.LVL209:
.LM330:
	pop r29
	pop r28
	ret
.LVL210:
.L229:
.LBB233:
.LBB234:
.LM331:
	mov r26,r24
	ldi r27,lo8(0)
	movw r28,r26
	subi r28,lo8(-(waveform))
	sbci r29,hi8(-(waveform))
	ld r24,Y
.LVL211:
	asr r24
	asr r24
	asr r24
	asr r24
	subi r24,lo8(-(9))
	andi r24,lo8(15)
.LBB235:
.LM332:
	mov r30,r24
	ldi r31,lo8(0)
.LVL212:
	subi r30,lo8(-(speakerPositions))
	sbci r31,hi8(-(speakerPositions))
.LVL213:
/* #APP */
 ;  480 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL214:
/* #NOAPP */
.LBE235:
	st Y,r30
.LM333:
	subi r26,lo8(-(frameBuffer))
	sbci r27,hi8(-(frameBuffer))
	st X,r24
.LVL215:
.LBE234:
.LBE233:
.LM334:
	pop r29
	pop r28
	ret
.LVL216:
.L230:
.LM335:
	call tuningEditorTouch
.LVL217:
.LM336:
	pop r29
	pop r28
	ret
.LFE33:
	.size	editorTouch, .-editorTouch
.global	savePattern
	.type	savePattern, @function
savePattern:
.LFB35:
.LM337:
.LVL218:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,9
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 9 */
.LM338:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(236)
	andi r27,hi8(236)
	lsl r26
	rol r27
	lsl r26
	rol r27
.LVL219:
	std Y+5,r27
	std Y+4,r26
.LVL220:
	std Y+2,__zero_reg__
	std Y+1,__zero_reg__
.LVL221:
.L237:
.LBB236:
.LM339:
	ldd r24,Y+1
	ldd r25,Y+2
	lsl r24
	rol r25
	lsl r24
	rol r25
	lsl r24
	rol r25
	std Y+7,r25
	std Y+6,r24
	std Y+3,__zero_reg__
.LVL222:
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L239:
.LBB237:
.LBB238:
.LBB239:
.LM340:
.LBE239:
	mov r30,r10
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	ldd r24,Y+6
	ldd r25,Y+7
	or r24,r26
	or r25,r27
	mov r31,r30
	clr r30
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(lightBuffers))
	sbci r31,hi8(-(lightBuffers))
	ld r24,Z
	tst r24
	breq .L238
.LM341:
	ldi r30,lo8(1)
	ldi r31,hi8(1)
	mov r0,r26
	rjmp 2f
1:	lsl r30
	rol r31
2:	dec r0
	brpl 1b
	ldd r31,Y+3
	or r31,r30
	std Y+3,r31
.LVL223:
.L238:
	adiw r26,1
.LM342:
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L239
.L243:
.LBE238:
.LBB240:
.LBB241:
.LM343:
	sbic 63-32,1
	rjmp .L243
.LM344:
	out 63-32,__zero_reg__
.LM345:
	ldd r24,Y+1
	ldd r25,Y+2
	andi r24,lo8(16)
	andi r25,hi8(16)
	lsl r24
	rol r25
	lsl r24
	rol r25
	ldd r30,Y+1
	ldd r31,Y+2
	andi r30,lo8(15)
	andi r31,hi8(15)
	add r24,r30
	adc r25,r31
	ldd r26,Y+4
	ldd r27,Y+5
	add r24,r26
	adc r25,r27
	out (65)+1-32,r25
	out 65-32,r24
.LM346:
	ldd r27,Y+3
	out 64-32,r27
.LM347:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
	ldd r30,Y+1
	ldd r31,Y+2
	adiw r30,1
	std Y+2,r31
	std Y+1,r30
.LBE241:
.LBE240:
.LBE237:
.LM348:
	sbiw r30,32
	breq .+2
	rjmp .L237
/* epilogue start */
.LBE236:
.LM349:
	adiw r28,9
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE35:
	.size	savePattern, .-savePattern
.global	saveCoarse
	.type	saveCoarse, @function
saveCoarse:
.LFB36:
.LM350:
.LVL224:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,11
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 11 */
.LM351:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(254)
	andi r27,hi8(254)
	lsl r26
	rol r27
	lsl r26
	rol r27
.LVL225:
	std Y+6,r27
	std Y+5,r26
	std Y+2,__zero_reg__
	std Y+1,__zero_reg__
.LVL226:
.L250:
.LBB242:
.LBB243:
.LBB244:
.LBB245:
.LM352:
	ldd r27,Y+1
.LVL227:
	swap r27
	lsl r27
	andi r27,lo8(-32)
	std Y+9,r27
.LVL228:
	ldi r26,lo8(0)
.LVL229:
	ldi r30,lo8(2048)
	ldi r31,hi8(2048)
	std Y+4,r31
	std Y+3,r30
.LVL230:
.L247:
.LBB246:
.LM353:
	ldd r30,Y+9
	or r30,r26
	ldi r31,lo8(0)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r24,Z
	mov r30,r24
	clr r31
	sbrc r30,7
	com r31
	ldd r24,Y+3
	ldd r25,Y+4
	add r24,r30
	adc r25,r31
	std Y+4,r25
	std Y+3,r24
.LVL231:
.LM354:
	subi r26,lo8(-(1))
	cpi r26,lo8(16)
	brne .L247
.LBE246:
.LBE245:
.LBE244:
.LBB247:
.LBB248:
.LM355:
	ldd r24,Y+1
	ldd r25,Y+2
	lsl r24
	rol r25
	mov r27,r24
.LVL232:
	ori r27,lo8(1)
	swap r27
	andi r27,lo8(-16)
	ldi r26,lo8(0)
.LVL233:
	ldi r30,lo8(2048)
	ldi r31,hi8(2048)
	std Y+8,r31
	std Y+7,r30
.LVL234:
.L248:
.LBB249:
.LM356:
	mov r30,r26
	or r30,r27
	ldi r31,lo8(0)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r24,Z
	clr r25
	sbrc r24,7
	com r25
	ldd r30,Y+7
	ldd r31,Y+8
	add r30,r24
	adc r31,r25
	std Y+8,r31
	std Y+7,r30
.LVL235:
.LM357:
	subi r26,lo8(-(1))
	cpi r26,lo8(16)
	brne .L248
.L252:
.LBE249:
.LBE248:
.LBE247:
.LBB250:
.LBB251:
.LM358:
	sbic 63-32,1
	rjmp .L252
.LM359:
	out 63-32,__zero_reg__
	ldd r24,Y+1
	ldd r25,Y+2
	ldd r26,Y+5
	ldd r27,Y+6
.LVL236:
	add r24,r26
	adc r25,r27
.LM360:
	out (65)+1-32,r25
	out 65-32,r24
.LBE251:
.LBE250:
.LM361:
	ldd r24,Y+7
	ldd r25,Y+8
	mov r30,r25
	clr r31
	sbrc r30,7
	dec r31
	swap r30
	andi r30,lo8(-16)
	ldd r26,Y+3
	ldd r27,Y+4
	mov r24,r27
	clr r25
	sbrc r24,7
	dec r25
	andi r24,lo8(15)
	or r30,r24
.LBB253:
.LBB252:
.LM362:
	out 64-32,r30
.LM363:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
	ldd r30,Y+1
	ldd r31,Y+2
	adiw r30,1
	std Y+2,r31
	std Y+1,r30
.LBE252:
.LBE253:
.LBE243:
.LM364:
	sbiw r30,8
	breq .+2
	rjmp .L250
/* epilogue start */
.LBE242:
.LM365:
	adiw r28,11
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE36:
	.size	saveCoarse, .-saveCoarse
.global	saveFine
	.type	saveFine, @function
saveFine:
.LFB37:
.LM366:
.LVL237:
	push r29
	push r28
	rcall .
	rcall .
	rcall .
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 6 */
.LM367:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(200)
	andi r27,hi8(200)
	lsl r26
	rol r27
	lsl r26
	rol r27
.LVL238:
	std Y+3,r27
	std Y+2,r26
.LVL239:
	std Y+1,__zero_reg__
.LVL240:
.L259:
.LBB254:
.LBB255:
.LM368:
	ldd r31,Y+1
	mov r30,r31
	ldi r31,lo8(0)
	std Y+6,r31
	std Y+5,r30
	lsl r30
	rol r31
	movw r26,r30
	subi r26,lo8(-(waveform))
	sbci r27,hi8(-(waveform))
	ld r24,X
	std Y+4,r24
	ori r30,lo8(1)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	ld r30,Z
.L258:
.LBB256:
.LBB257:
.LM369:
	sbic 63-32,1
	rjmp .L258
.LM370:
	out 63-32,__zero_reg__
.LM371:
	ldd r24,Y+5
	ldd r25,Y+6
	andi r24,lo8(96)
	andi r25,hi8(96)
	lsl r24
	rol r25
	ldd r26,Y+5
	ldd r27,Y+6
	andi r26,lo8(31)
	andi r27,hi8(31)
	add r24,r26
	adc r25,r27
	ldd r26,Y+2
	ldd r27,Y+3
	add r24,r26
	adc r25,r27
	out (65)+1-32,r25
	out 65-32,r24
.LBE257:
.LBE256:
.LM372:
	andi r30,lo8(-16)
	ldd r27,Y+4
	asr r27
	asr r27
	asr r27
	asr r27
	or r30,r27
	ldi r31,lo8(-120)
	eor r30,r31
.LBB259:
.LBB258:
.LM373:
	out 64-32,r30
.LM374:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.LBE258:
.LBE259:
.LBE255:
.LM375:
	ldd r24,Y+1
	subi r24,lo8(-(1))
	std Y+1,r24
.LVL241:
	cpi r24,lo8(-128)
	breq .+2
	rjmp .L259
/* epilogue start */
.LBE254:
.LM376:
	adiw r28,6
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE37:
	.size	saveFine, .-saveFine
.global	saveTuning
	.type	saveTuning, @function
saveTuning:
.LFB38:
.LM377:
.LVL242:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM378:
	mov r26,r24
.LVL243:
	ldi r27,lo8(0)
.LVL244:
	andi r26,lo8(236)
	andi r27,hi8(236)
.LVL245:
	lsl r26
	rol r27
	lsl r26
	rol r27
	ldi r30,lo8(frequencies)
	ldi r31,hi8(frequencies)
	movw r28,r26
	subi r28,lo8(-(64))
	sbci r29,hi8(-(64))
.L266:
.LBB260:
.LM379:
	ld r25,Z
	ldd r24,Z+1
.LVL246:
.L264:
.LBB261:
.LBB262:
.LM380:
	sbic 63-32,1
	rjmp .L264
.LM381:
	out 63-32,__zero_reg__
.LM382:
	out (65)+1-32,r27
	out 65-32,r26
.LM383:
	out 64-32,r25
.LM384:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.L265:
.LBE262:
.LBE261:
.LBB263:
.LBB264:
.LM385:
	sbic 63-32,1
	rjmp .L265
.LM386:
	out 63-32,__zero_reg__
.LM387:
	out (65)+1-32,r29
	out 65-32,r28
.LM388:
	out 64-32,r24
.LM389:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
	adiw r30,2
.LBE264:
.LBE263:
.LM390:
	ldi r24,hi8(frequencies+32)
	cpi r30,lo8(frequencies+32)
	cpc r31,r24
	brne .L266
/* epilogue start */
.LBE260:
.LM391:
	pop r29
	pop r28
	ret
.LFE38:
	.size	saveTuning, .-saveTuning
.global	saveMiscellaneous
	.type	saveMiscellaneous, @function
saveMiscellaneous:
.LFB39:
.LM392:
.LVL247:
/* prologue: function */
/* frame size = 0 */
.LM393:
	ldi r25,lo8(0)
.LVL248:
	andi r24,lo8(254)
	andi r25,hi8(254)
.LVL249:
	lsl r24
	rol r25
	lsl r24
	rol r25
.LM394:
	lds r30,framesPerBeatOverride
	lds r31,framesPerBeatOverride+1
.L272:
.LBB265:
.LBB266:
.LM395:
	sbic 63-32,1
	rjmp .L272
.LM396:
	out 63-32,__zero_reg__
.LM397:
	out (65)+1-32,r25
	out 65-32,r24
.LM398:
	out 64-32,r30
.LM399:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.L273:
.LBE266:
.LBE265:
.LBB267:
.LBB268:
.LM400:
	sbic 63-32,1
	rjmp .L273
.LM401:
	out 63-32,__zero_reg__
.LM402:
	adiw r24,1
	out (65)+1-32,r25
	out 65-32,r24
	sbiw r24,1
.LM403:
	out 64-32,r31
.LM404:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.LBE268:
.LBE267:
.LM405:
	lds r30,decayConstantOverride
.L274:
.LBB269:
.LBB270:
.LM406:
	sbic 63-32,1
	rjmp .L274
.LM407:
	out 63-32,__zero_reg__
.LM408:
	adiw r24,2
	out (65)+1-32,r25
	out 65-32,r24
	sbiw r24,2
.LM409:
	out 64-32,r30
.LM410:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.LBE270:
.LBE269:
.LM411:
	mov r30,r14
.L275:
.LBB271:
.LBB272:
.LM412:
	sbic 63-32,1
	rjmp .L275
.LM413:
	out 63-32,__zero_reg__
.LM414:
	adiw r24,3
	out (65)+1-32,r25
	out 65-32,r24
	sbiw r24,3
.LM415:
	out 64-32,r30
.LM416:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.LBE272:
.LBE271:
.LM417:
	lds r30,patternsPerLoop
.L276:
.LBB273:
.LBB274:
.LM418:
	sbic 63-32,1
	rjmp .L276
.LM419:
	out 63-32,__zero_reg__
.LM420:
	adiw r24,4
	out (65)+1-32,r25
	out 65-32,r24
	sbiw r24,4
.LM421:
	out 64-32,r30
.LM422:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.LBE274:
.LBE273:
.LM423:
	lds r31,flags2
	mov r30,r31
	lsr r30
	lsr r30
	lsr r30
	andi r30,lo8(1)
.L277:
.LBB275:
.LBB276:
.LM424:
	sbic 63-32,1
	rjmp .L277
.LM425:
	out 63-32,__zero_reg__
.LM426:
	adiw r24,5
	out (65)+1-32,r25
	out 65-32,r24
	sbiw r24,5
.LM427:
	out 64-32,r30
.LM428:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* #NOAPP */
.LBE276:
.LBE275:
.LM429:
	mov r30,r31
	swap r30
	andi r30,lo8(1)
.L278:
.LBB277:
.LBB278:
.LM430:
	sbic 63-32,1
	rjmp .L278
.LM431:
	out 63-32,__zero_reg__
.LM432:
	adiw r24,6
	out (65)+1-32,r25
	out 65-32,r24
.LM433:
	out 64-32,r30
.LM434:
/* #APP */
 ;  315 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM WRITE CRITICAL SECTION */
	in	r0, 63		
	cli				
	sbi	31, 2	
	sbi	31, 1	
	out	63, r0		
	/* END EEPROM WRITE CRITICAL SECTION */
 ;  0 "" 2
/* epilogue start */
/* #NOAPP */
.LBE278:
.LBE277:
.LM435:
	ret
.LFE39:
	.size	saveMiscellaneous, .-saveMiscellaneous
.global	startEscapeMode
	.type	startEscapeMode, @function
startEscapeMode:
.LFB40:
.LM436:
/* prologue: function */
/* frame size = 0 */
.LM437:
	ldi r24,lo8(1)
	sts mode,r24
.LM438:
/* #APP */
 ;  712 "v2c.c" 1
	andi r17, 0xbf
 ;  0 "" 2
.LM439:
/* #NOAPP */
	call drawEscapeMenu
/* epilogue start */
.LM440:
	ret
.LFE40:
	.size	startEscapeMode, .-startEscapeMode
.global	save
	.type	save, @function
save:
.LFB41:
.LM441:
.LVL250:
/* prologue: function */
/* frame size = 0 */
.LM442:
/* #APP */
 ;  726 "v2c.c" 1
	cli
 ;  0 "" 2
.LM443:
/* #NOAPP */
	lds r25,editor
	cpi r25,lo8(2)
	breq .L293
	cpi r25,lo8(3)
	brlo .L298
	cpi r25,lo8(3)
	breq .L294
	cpi r25,lo8(4)
	breq .L299
.L290:
.LM444:
/* #APP */
 ;  734 "v2c.c" 1
	sei
 ;  0 "" 2
.LM445:
/* #NOAPP */
	call startEscapeMode
.LVL251:
/* epilogue start */
.LM446:
	ret
.LVL252:
.L298:
.LM447:
	tst r25
	breq .L291
	cpi r25,lo8(1)
	brne .L290
.LM448:
	call saveCoarse
.LVL253:
.LM449:
/* #APP */
 ;  734 "v2c.c" 1
	sei
 ;  0 "" 2
.LM450:
/* #NOAPP */
	call startEscapeMode
.LM451:
	ret
.LVL254:
.L291:
.LM452:
	call savePattern
.LVL255:
.LM453:
/* #APP */
 ;  734 "v2c.c" 1
	sei
 ;  0 "" 2
.LM454:
/* #NOAPP */
	call startEscapeMode
.LM455:
	ret
.LVL256:
.L299:
.LM456:
	call saveMiscellaneous
.LVL257:
.LM457:
/* #APP */
 ;  734 "v2c.c" 1
	sei
 ;  0 "" 2
.LM458:
/* #NOAPP */
	call startEscapeMode
.LM459:
	ret
.LVL258:
.L293:
.LM460:
	call saveFine
.LVL259:
.LM461:
/* #APP */
 ;  734 "v2c.c" 1
	sei
 ;  0 "" 2
.LM462:
/* #NOAPP */
	call startEscapeMode
.LM463:
	ret
.LVL260:
.L294:
.LM464:
	call saveTuning
.LVL261:
.LM465:
/* #APP */
 ;  734 "v2c.c" 1
	sei
 ;  0 "" 2
.LM466:
/* #NOAPP */
	call startEscapeMode
.LM467:
	ret
.LFE41:
	.size	save, .-save
.global	loadPattern
	.type	loadPattern, @function
loadPattern:
.LFB42:
.LM468:
.LVL262:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,14
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 14 */
.LM469:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(236)
	andi r27,hi8(236)
	lsl r26
	rol r27
	lsl r26
	rol r27
.LVL263:
	std Y+9,r27
	std Y+8,r26
.LVL264:
	std Y+5,__zero_reg__
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL265:
.L309:
.LBB279:
.LBB280:
.LBB281:
.LBB282:
.LM470:
	sbic 63-32,1
	rjmp .L309
.LM471:
	ldd r24,Y+3
	ldd r25,Y+4
	andi r24,lo8(16)
	andi r25,hi8(16)
	lsl r24
	rol r25
	lsl r24
	rol r25
	ldd r30,Y+3
	ldd r31,Y+4
	andi r30,lo8(15)
	andi r31,hi8(15)
	add r24,r30
	adc r25,r31
	ldd r30,Y+8
	ldd r31,Y+9
	add r24,r30
	adc r25,r31
	out (65)+1-32,r25
	out 65-32,r24
.LM472:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL266:
/* #NOAPP */
	mov r26,r24
	ldi r27,lo8(0)
	std Y+2,r27
	std Y+1,r26
	ldd r30,Y+3
	ldd r31,Y+4
	lsl r30
	rol r31
	lsl r30
	rol r31
	lsl r30
	rol r31
	std Y+7,r31
	std Y+6,r30
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L304:
.LBE282:
.LBE281:
.LBB283:
.LBB284:
.LM473:
	ldd r24,Y+1
	ldd r25,Y+2
.LVL267:
	mov r0,r26
	rjmp 2f
1:	asr r25
	ror r24
2:	dec r0
	brpl 1b
.LM474:
	mov r25,r24
.LVL268:
	andi r25,lo8(1)
	sbrs r24,0
	rjmp .L302
.LM475:
	ldd r31,Y+5
	subi r31,lo8(-(1))
	std Y+5,r31
.L302:
.LBB285:
.LM476:
.LBE285:
	mov r30,r10
	ldi r31,lo8(0)
	andi r30,lo8(1)
	andi r31,hi8(1)
	std Y+13,r31
	std Y+12,r30
	ldd r30,Y+6
	ldd r31,Y+7
	or r30,r26
	or r31,r27
	std Y+11,r31
	std Y+10,r30
	std Y+14,r25
	tst r25
	breq .L303
	ldi r31,lo8(1)
	std Y+14,r31
.L303:
	ldd r24,Y+12
	ldd r25,Y+13
	mov r31,r24
	clr r30
	ldd r24,Y+10
	ldd r25,Y+11
	add r30,r24
	adc r31,r25
	subi r30,lo8(-(lightBuffers))
	sbci r31,hi8(-(lightBuffers))
	ldd r25,Y+14
.LVL269:
	st Z,r25
	adiw r26,1
.LBE284:
.LM477:
	cpi r26,8
	cpc r27,__zero_reg__
	brne .L304
	ldd r26,Y+3
	ldd r27,Y+4
	adiw r26,1
	std Y+4,r27
	std Y+3,r26
.LBE283:
.LBE280:
.LM478:
	sbiw r26,32
	breq .+2
	rjmp .L309
	ldd r16,Y+5
/* epilogue start */
.LBE279:
.LM479:
	adiw r28,14
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE42:
	.size	loadPattern, .-loadPattern
.global	loadCoarse
	.type	loadCoarse, @function
loadCoarse:
.LFB43:
.LM480:
.LVL270:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,8
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 8 */
.LM481:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(254)
	andi r27,hi8(254)
	lsl r26
	rol r27
	lsl r26
	rol r27
.LVL271:
	std Y+6,r27
	std Y+5,r26
	std Y+4,__zero_reg__
	std Y+3,__zero_reg__
.LVL272:
.L317:
.LBB286:
.LBB287:
.LBB288:
.LBB289:
.LM482:
	sbic 63-32,1
	rjmp .L317
	ldd r24,Y+3
	ldd r25,Y+4
	ldd r30,Y+5
	ldd r31,Y+6
	add r24,r30
	adc r25,r31
.LM483:
	out (65)+1-32,r25
	out 65-32,r24
.LM484:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r30, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL273:
/* #NOAPP */
	ldd r24,Y+3
	ldd r25,Y+4
	lsl r24
	rol r25
	swap r24
	swap r25
	andi r25,0xf0
	eor r25,r24
	andi r24,0xf0
	eor r25,r24
	std Y+2,r25
	std Y+1,r24
	mov r24,r30
	ldi r25,lo8(0)
.LVL274:
	andi r24,lo8(15)
	andi r25,hi8(15)
	subi r24,lo8(-(speakerPositions))
	sbci r25,hi8(-(speakerPositions))
	swap r30
	andi r30,lo8(15)
	mov r26,r30
	ldi r27,lo8(0)
	subi r26,lo8(-(speakerPositions))
	sbci r27,hi8(-(speakerPositions))
	std Y+8,__zero_reg__
	std Y+7,__zero_reg__
.LBE289:
.LBE288:
.LBB290:
.LBB291:
.LM485:
	movw r30,r24
.LVL275:
/* #APP */
 ;  761 "v2c.c" 1
	lpm r25, Z
	
 ;  0 "" 2
.LVL276:
/* #NOAPP */
.LBE291:
.LBB292:
.LM486:
	movw r30,r26
/* #APP */
 ;  762 "v2c.c" 1
	lpm r24, Z
	
 ;  0 "" 2
.LVL277:
/* #NOAPP */
.L312:
.LBE292:
.LM487:
	ldd r30,Y+1
	ldd r31,Y+2
	ldd r26,Y+7
	ldd r27,Y+8
	or r30,r26
	or r31,r27
	movw r26,r30
	subi r26,lo8(-(waveform))
	sbci r27,hi8(-(waveform))
	st X,r25
.LM488:
	ori r30,lo8(16)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	st Z,r24
	ldd r30,Y+7
	ldd r31,Y+8
	adiw r30,1
	std Y+8,r31
	std Y+7,r30
.LM489:
	sbiw r30,16
	brne .L312
	ldd r24,Y+3
	ldd r25,Y+4
.LVL278:
	adiw r24,1
	std Y+4,r25
	std Y+3,r24
.LBE290:
.LBE287:
.LM490:
	sbiw r24,8
	breq .+2
	rjmp .L317
/* epilogue start */
.LBE286:
.LM491:
	adiw r28,8
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE43:
	.size	loadCoarse, .-loadCoarse
.global	loadFine
	.type	loadFine, @function
loadFine:
.LFB44:
.LM492:
.LVL279:
	push r29
	push r28
	rcall .
	rcall .
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 4 */
.LM493:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(200)
	andi r27,hi8(200)
	lsl r26
	rol r27
	lsl r26
	rol r27
.LVL280:
	std Y+4,r27
	std Y+3,r26
.LVL281:
	std Y+2,__zero_reg__
.LVL282:
.L320:
.LBB293:
.LBB294:
.LM494:
	ldd r30,Y+2
.LVL283:
	mov r26,r30
.LVL284:
	ldi r27,lo8(0)
.LVL285:
.L319:
.LBB295:
.LBB296:
.LM495:
	sbic 63-32,1
	rjmp .L319
.LM496:
	movw r24,r26
	andi r24,lo8(96)
	andi r25,hi8(96)
	lsl r24
	rol r25
	movw r30,r26
	andi r30,lo8(31)
	andi r31,hi8(31)
	add r24,r30
	adc r25,r31
	ldd r30,Y+3
	ldd r31,Y+4
	add r24,r30
	adc r25,r31
	out (65)+1-32,r25
	out 65-32,r24
.LM497:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r31, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
/* #NOAPP */
	std Y+1,r31
.LVL286:
.LBE296:
.LBE295:
.LM498:
	movw r24,r26
	lsl r24
	rol r25
.LBB297:
	mov r30,r31
.LVL287:
	ldi r31,lo8(0)
.LVL288:
	andi r30,lo8(15)
	andi r31,hi8(15)
.LVL289:
	subi r30,lo8(-(speakerPositions))
	sbci r31,hi8(-(speakerPositions))
.LVL290:
/* #APP */
 ;  773 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL291:
/* #NOAPP */
.LBE297:
	movw r26,r24
.LVL292:
	subi r26,lo8(-(waveform))
	sbci r27,hi8(-(waveform))
	st X,r30
.LBB298:
.LM499:
	ldd r26,Y+1
.LVL293:
	swap r26
	andi r26,lo8(15)
	mov r30,r26
.LVL294:
	ldi r31,lo8(0)
.LVL295:
	subi r30,lo8(-(speakerPositions))
	sbci r31,hi8(-(speakerPositions))
/* #APP */
 ;  774 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL296:
/* #NOAPP */
.LBE298:
	ori r24,lo8(1)
	subi r24,lo8(-(waveform))
	sbci r25,hi8(-(waveform))
	movw r26,r24
	st X,r30
.LBE294:
.LM500:
	ldd r27,Y+2
.LVL297:
	subi r27,lo8(-(1))
	std Y+2,r27
.LVL298:
	cpi r27,lo8(-128)
	breq .+2
	rjmp .L320
/* epilogue start */
.LBE293:
.LM501:
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop __tmp_reg__
	pop r28
	pop r29
	ret
.LFE44:
	.size	loadFine, .-loadFine
.global	loadTuning
	.type	loadTuning, @function
loadTuning:
.LFB45:
.LM502:
.LVL299:
	push r29
	push r28
	in r28,__SP_L__
	in r29,__SP_H__
	sbiw r28,7
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
/* prologue: function */
/* frame size = 7 */
.LM503:
	mov r26,r24
	ldi r27,lo8(0)
	andi r26,lo8(236)
	andi r27,hi8(236)
	lsl r26
	rol r27
	lsl r26
	rol r27
	std Y+7,r27
	std Y+6,r26
.LVL300:
	subi r26,lo8(-(64))
	sbci r27,hi8(-(64))
	std Y+2,r27
	std Y+1,r26
	ldi r30,lo8(frequencies)
	ldi r31,hi8(frequencies)
	std Y+5,r31
	std Y+4,r30
.LVL301:
.L331:
.LBB299:
.LBB300:
.LBB301:
.LM504:
	sbic 63-32,1
	rjmp .L331
.LM505:
	ldd r24,Y+1
	ldd r25,Y+2
.LVL302:
	out (65)+1-32,r25
	out 65-32,r24
.LM506:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r25, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL303:
/* #NOAPP */
	std Y+3,r25
.LVL304:
.L326:
.LBE301:
.LBE300:
.LBB302:
.LBB303:
.LM507:
	sbic 63-32,1
	rjmp .L326
.LM508:
	ldd r26,Y+6
	ldd r27,Y+7
	out (65)+1-32,r27
	out 65-32,r26
.LM509:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL305:
/* #NOAPP */
.LBE303:
.LBE302:
.LM510:
	ldi r25,lo8(0)
.LVL306:
	ldd r31,Y+3
	ldi r30,lo8(0)
.LVL307:
	or r24,r30
	or r25,r31
.LVL308:
	ldd r26,Y+4
	ldd r27,Y+5
	st X+,r24
	st X+,r25
	std Y+5,r27
	std Y+4,r26
.LM511:
	subi r26,lo8(frequencies+32)
	sbci r27,hi8(frequencies+32)
.LVL309:
	brne .L331
/* epilogue start */
.LBE299:
.LM512:
	adiw r28,7
	in __tmp_reg__,__SREG__
	cli
	out __SP_H__,r29
	out __SREG__,__tmp_reg__
	out __SP_L__,r28
	pop r28
	pop r29
	ret
.LFE45:
	.size	loadTuning, .-loadTuning
.global	loadMiscellaneous
	.type	loadMiscellaneous, @function
loadMiscellaneous:
.LFB46:
.LM513:
.LVL310:
/* prologue: function */
/* frame size = 0 */
.LM514:
	mov r26,r24
.LVL311:
	ldi r27,lo8(0)
.LVL312:
	andi r26,lo8(254)
	andi r27,hi8(254)
.LVL313:
	lsl r26
	rol r27
	lsl r26
	rol r27
.L333:
.LBB304:
.LBB305:
.LM515:
	sbic 63-32,1
	rjmp .L333
.LM516:
	out (65)+1-32,r27
	out 65-32,r26
.LM517:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r25, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL314:
/* #NOAPP */
.L334:
.LBE305:
.LBE304:
.LBB306:
.LBB307:
.LM518:
	sbic 63-32,1
	rjmp .L334
.LM519:
	adiw r26,1
	out (65)+1-32,r27
	out 65-32,r26
	sbiw r26,1
.LM520:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL315:
/* #NOAPP */
.LBE307:
.LBE306:
.LM521:
	mov r31,r24
.LVL316:
	ldi r30,lo8(0)
.LVL317:
	mov r24,r25
	ldi r25,lo8(0)
.LVL318:
	or r30,r24
	or r31,r25
.LVL319:
	sts (framesPerBeatOverride)+1,r31
	sts framesPerBeatOverride,r30
.L335:
.LBB308:
.LBB309:
.LM522:
	sbic 63-32,1
	rjmp .L335
.LM523:
	adiw r26,2
	out (65)+1-32,r27
	out 65-32,r26
	sbiw r26,2
.LM524:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL320:
/* #NOAPP */
.LBE309:
.LBE308:
.LM525:
	sts decayConstantOverride,r24
.L336:
.LBB310:
.LBB311:
.LM526:
	sbic 63-32,1
	rjmp .L336
.LM527:
	adiw r26,3
	out (65)+1-32,r27
	out 65-32,r26
	sbiw r26,3
.LM528:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL321:
/* #NOAPP */
.LBE311:
.LBE310:
.LM529:
	mov r14,r24
.L337:
.LBB312:
.LBB313:
.LM530:
	sbic 63-32,1
	rjmp .L337
.LM531:
	adiw r26,4
	out (65)+1-32,r27
	out 65-32,r26
	sbiw r26,4
.LM532:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL322:
/* #NOAPP */
.LBE313:
.LBE312:
.LM533:
	sts patternsPerLoop,r24
.L338:
.LBB314:
.LBB315:
.LM534:
	sbic 63-32,1
	rjmp .L338
.LM535:
	adiw r26,5
	out (65)+1-32,r27
	out 65-32,r26
	sbiw r26,5
.LM536:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL323:
/* #NOAPP */
.LBE315:
.LBE314:
.LM537:
	andi r24,lo8(1)
	lsl r24
	lsl r24
	lsl r24
	lds r25,flags2
.LVL324:
	andi r25,lo8(-9)
	or r25,r24
	sts flags2,r25
.L339:
.LBB316:
.LBB317:
.LM538:
	sbic 63-32,1
	rjmp .L339
.LM539:
	adiw r26,6
	out (65)+1-32,r27
	out 65-32,r26
.LM540:
/* #APP */
 ;  208 "c:/program files/arduino/hardware/tools/avr/lib/gcc/../../avr/include/avr/eeprom.h" 1
	/* START EEPROM READ CRITICAL SECTION */ 
	sbi 31, 0 
	in r24, 32 
	/* END EEPROM READ CRITICAL SECTION */ 
	
 ;  0 "" 2
.LVL325:
/* #NOAPP */
.LBE317:
.LBE316:
.LM541:
	andi r24,lo8(1)
	swap r24
	andi r24,lo8(-16)
	andi r25,lo8(-17)
	or r25,r24
	sts flags2,r25
/* epilogue start */
.LM542:
	ret
.LFE46:
	.size	loadMiscellaneous, .-loadMiscellaneous
.global	load
	.type	load, @function
load:
.LFB47:
.LM543:
.LVL326:
/* prologue: function */
/* frame size = 0 */
.LM544:
	lds r25,editor
	cpi r25,lo8(2)
	breq .L352
	cpi r25,lo8(3)
	brlo .L357
	cpi r25,lo8(3)
	breq .L353
	cpi r25,lo8(4)
	breq .L358
.L349:
.LM545:
	call startEscapeMode
.LVL327:
/* epilogue start */
.LM546:
	ret
.LVL328:
.L357:
.LM547:
	tst r25
	breq .L350
	cpi r25,lo8(1)
	brne .L349
.LM548:
	call loadCoarse
.LVL329:
.LM549:
	call startEscapeMode
.LM550:
	ret
.LVL330:
.L350:
.LM551:
	call loadPattern
.LVL331:
.LM552:
	call startEscapeMode
.LM553:
	ret
.LVL332:
.L358:
.LM554:
	call loadMiscellaneous
.LVL333:
.LM555:
	call startEscapeMode
.LM556:
	ret
.LVL334:
.L352:
.LM557:
	call loadFine
.LVL335:
.LM558:
	call startEscapeMode
.LM559:
	ret
.LVL336:
.L353:
.LM560:
	call loadTuning
.LVL337:
.LM561:
	call startEscapeMode
.LM562:
	ret
.LFE47:
	.size	load, .-load
.global	escapeModeTouch
	.type	escapeModeTouch, @function
escapeModeTouch:
.LFB34:
.LM563:
.LVL338:
	push r28
	push r29
/* prologue: function */
/* frame size = 0 */
.LM564:
	ldi r25,lo8(0)
	movw r30,r24
.LVL339:
	andi r30,lo8(192)
	andi r31,hi8(192)
	asr r31
	ror r30
	asr r31
	ror r30
	asr r31
	ror r30
	asr r31
	ror r30
	andi r24,lo8(12)
	andi r25,hi8(12)
	asr r25
	ror r24
	asr r25
	ror r24
	or r30,r24
	or r31,r25
	cpi r30,7
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L368
	cpi r30,8
	cpc r31,__zero_reg__
	brlt .L401
	cpi r30,11
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L372
	cpi r30,12
	cpc r31,__zero_reg__
	brge .+2
	rjmp .L402
	cpi r30,13
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L374
	cpi r30,13
	cpc r31,__zero_reg__
	brge .+2
	rjmp .L373
	sbiw r30,14
	brne .L390
.LM565:
	lds r24,flags2
	ldi r25,lo8(2)
	eor r24,r25
	sts flags2,r24
.LVL340:
.L400:
.LM566:
	call drawEscapeMenu
.LVL341:
.L390:
/* epilogue start */
.LM567:
	pop r29
	pop r28
	ret
.L401:
.LM568:
	cpi r30,3
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L364
	cpi r30,4
	cpc r31,__zero_reg__
	brge .L377
	cpi r30,1
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L362
	cpi r30,2
	cpc r31,__zero_reg__
	brge .+2
	rjmp .L403
.LM569:
	ldi r24,lo8(2)
	sts waveformPreset,r24
	ldi r30,lo8(waveform)
	ldi r31,hi8(waveform)
	ldi r25,lo8(0)
.L382:
.LM570:
	mov r24,r25
	lsl r24
	subi r24,lo8(-(-128))
	st Z+,r24
	subi r25,lo8(-(1))
.LM571:
	cpi r25,lo8(-128)
	brne .L382
	ldi r30,lo8(waveform+128)
	ldi r31,hi8(waveform+128)
	ldi r25,lo8(0)
.LM572:
	ldi r26,lo8(127)
.L383:
	mov r24,r25
	lsl r24
	mov r27,r26
	sub r27,r24
	st Z+,r27
	subi r25,lo8(-(1))
.LM573:
	cpi r25,lo8(-128)
	brne .L383
	rjmp .L400
.L402:
.LM574:
	cpi r30,9
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L370
	sbiw r30,10
	brlt .L404
.LM575:
	ldi r24,lo8(2)
	sts mode,r24
.LM576:
	call drawEEPROMScreen
	rjmp .L390
.L377:
.LM577:
	cpi r30,5
	cpc r31,__zero_reg__
	brne .+2
	rjmp .L366
	sbiw r30,6
	brge .L367
.LM578:
	ldi r24,lo8(4)
	sts waveformPreset,r24
	ldi r28,lo8(waveform)
	ldi r29,hi8(waveform)
.L385:
.LM579:
	call randomByte
	st Y+,r24
.LM580:
	ldi r24,hi8(waveform+256)
	cpi r28,lo8(waveform+256)
	cpc r29,r24
	brne .L385
	rjmp .L400
.L367:
.LM581:
	ldi r24,lo8(1)
	sts editor,r24
.LM582:
	call drawEscapeMenu
	rjmp .L390
.L404:
.LM583:
	ldi r24,lo8(3)
	sts editor,r24
.LM584:
	call drawEscapeMenu
	rjmp .L390
.L373:
.LBB318:
.LM585:
.LVL342:
.LBE318:
	sbrs r17,1
	rjmp .L386
.LM586:
/* #APP */
 ;  632 "v2c.c" 1
	andi r17, 0xfd
 ;  0 "" 2
/* #NOAPP */
	rjmp .L400
.LVL343:
.L374:
.LBB319:
.LM587:
.LVL344:
.LBE319:
	sbrs r17,5
	rjmp .L388
.LM588:
/* #APP */
 ;  639 "v2c.c" 1
	andi r17, 0xdf
 ;  0 "" 2
/* #NOAPP */
	rjmp .L400
.LVL345:
.L364:
.LM589:
	ldi r24,lo8(3)
	sts waveformPreset,r24
	ldi r24,lo8(0)
	ldi r25,hi8(0)
.LVL346:
.L384:
.LM590:
	movw r30,r24
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	st Z,r24
.LM591:
	adiw r24,1
	ldi r30,hi8(256)
	cpi r24,lo8(256)
	cpc r25,r30
	brne .L384
	rjmp .L400
.LVL347:
.L368:
.LM592:
	ldi r24,lo8(2)
	sts editor,r24
.LM593:
	call drawEscapeMenu
	rjmp .L390
.L372:
.LM594:
	ldi r24,lo8(3)
	sts mode,r24
.LM595:
	call drawEEPROMScreen
	rjmp .L390
.L370:
.LM596:
	ldi r24,lo8(4)
	sts editor,r24
.LM597:
	call drawEscapeMenu
	rjmp .L390
.L362:
.LM598:
	ldi r24,lo8(1)
	sts waveformPreset,r24
	ldi r30,lo8(waveform)
	ldi r31,hi8(waveform)
.LM599:
	ldi r24,lo8(-128)
.L380:
	st Z+,r24
.LM600:
	ldi r25,hi8(waveform+128)
	cpi r30,lo8(waveform+128)
	cpc r31,r25
	brne .L380
.LM601:
	ldi r24,lo8(127)
.L381:
	st Z+,r24
.LM602:
	ldi r27,hi8(waveform+256)
	cpi r30,lo8(waveform+256)
	cpc r31,r27
	brne .L381
	rjmp .L400
.L366:
.LM603:
	sts editor,__zero_reg__
.LM604:
	call drawEscapeMenu
	rjmp .L390
.LVL348:
.L388:
.LM605:
/* #APP */
 ;  641 "v2c.c" 1
	ori r17, 0x20
 ;  0 "" 2
/* #NOAPP */
	rjmp .L400
.LVL349:
.L386:
.LM606:
/* #APP */
 ;  634 "v2c.c" 1
	ori r17, 2
 ;  0 "" 2
/* #NOAPP */
	rjmp .L400
.LVL350:
.L403:
.LM607:
	or r30,r31
	breq .+2
	rjmp .L390
.LM608:
	sts waveformPreset,__zero_reg__
	ldi r24,lo8(0)
	ldi r25,hi8(0)
.LVL351:
.L379:
.LBB320:
.LBB321:
.LBB322:
.LBB323:
.LM609:
	movw r30,r24
	subi r30,lo8(-(sineTable))
	sbci r31,hi8(-(sineTable))
/* #APP */
 ;  290 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL352:
/* #NOAPP */
.LBE323:
	movw r26,r24
	subi r26,lo8(-(waveform))
	sbci r27,hi8(-(waveform))
	st X,r30
.LM610:
	adiw r24,1
	ldi r27,hi8(256)
	cpi r24,lo8(256)
	cpc r25,r27
	brne .L379
	rjmp .L400
.LBE322:
.LBE321:
.LBE320:
.LFE34:
	.size	escapeModeTouch, .-escapeModeTouch
.global	touch
	.type	touch, @function
touch:
.LFB48:
.LM611:
.LVL353:
/* prologue: function */
/* frame size = 0 */
.LM612:
	lds r25,mode
	cpi r25,lo8(1)
	breq .L408
	cpi r25,lo8(1)
	brlo .L407
	cpi r25,lo8(2)
	breq .L409
	cpi r25,lo8(3)
	breq .L412
	ret
.L407:
.LM613:
	call editorTouch
.LVL354:
	ret
.LVL355:
.L412:
.LM614:
	call load
.LVL356:
	ret
.LVL357:
.L408:
.LM615:
	call escapeModeTouch
.LVL358:
	ret
.LVL359:
.L409:
.LM616:
	call save
.LVL360:
	ret
.LFE48:
	.size	touch, .-touch
.global	idleLoop
	.type	idleLoop, @function
idleLoop:
.LFB49:
.LM617:
	push r29
	push r28
	push __tmp_reg__
	in r28,__SP_L__
	in r29,__SP_H__
/* prologue: function */
/* frame size = 1 */
.LM618:
	lds r24,122
	sbrs r24,6
	rjmp .L428
.LVL361:
.L414:
.LM619:
	sbis 35-32,4
	rjmp .L415
.L430:
.LM620:
	lds r25,flags2
	andi r25,lo8(-65)
	sts flags2,r25
.L416:
.LBB324:
.LM621:
.LVL362:
.LBE324:
	tst r13
	brne .L418
.LM622:
	andi r25,lo8(-2)
	sts flags2,r25
.L418:
.LM623:
	sbrs r25,1
	rjmp .L419
	ldi r26,lo8(0)
	ldi r27,hi8(0)
.L421:
.LBB325:
.LM624:
	movw r30,r26
.LVL363:
	subi r30,lo8(-(microtoneKeys))
	sbci r31,hi8(-(microtoneKeys))
	ld r30,Z
	ldi r31,lo8(0)
.LVL364:
	subi r30,lo8(-(switchBuffer))
	sbci r31,hi8(-(switchBuffer))
	ld r24,Z
	tst r24
	breq .L420
.LM625:
	movw r30,r26
	subi r30,lo8(-(volumes))
	sbci r31,hi8(-(volumes))
	ldi r24,lo8(-1)
	st Z,r24
.L420:
	adiw r26,1
.LM626:
	cpi r26,16
	cpc r27,__zero_reg__
	brne .L421
.LVL365:
.L419:
.LBE325:
.LBB326:
.LM627:
.LVL366:
.LBE326:
	sbrc r17,7
	rjmp .L429
.L422:
.LM628:
	sbrs r25,4
	rjmp .L423
.LM629:
	lds r25,noiseUpdatePointer
	std Y+1,r25
	call randomByte
.LVL367:
	ldd r25,Y+1
	mov r30,r25
	ldi r31,lo8(0)
	subi r30,lo8(-(waveform))
	sbci r31,hi8(-(waveform))
	st Z,r24
	subi r25,lo8(-(1))
	sts noiseUpdatePointer,r25
.L424:
.LBB327:
.LBB328:
.LM630:
.LVL368:
.LBE328:
	mov r30,r9
	ldi r31,lo8(0)
	subi r30,lo8(-(redLED))
	sbci r31,hi8(-(redLED))
/* #APP */
 ;  865 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL369:
/* #NOAPP */
.LBE327:
	out 72-32,r30
.LBB329:
.LBB330:
.LM631:
.LVL370:
.LBE330:
	mov r30,r9
.LVL371:
	ldi r31,lo8(0)
	subi r30,lo8(-(greenLED))
	sbci r31,hi8(-(greenLED))
/* #APP */
 ;  866 "v2c.c" 1
	lpm r30, Z
	
 ;  0 "" 2
.LVL372:
/* #NOAPP */
.LBE329:
	out 71-32,r30
.LBB331:
.LBB332:
.LM632:
.LVL373:
.LBE332:
	mov r30,r9
.LVL374:
	ldi r31,lo8(0)
.LVL375:
	lsl r30
	rol r31
.LVL376:
	subi r30,lo8(-(blueLED))
	sbci r31,hi8(-(blueLED))
/* #APP */
 ;  867 "v2c.c" 1
	lpm r24, Z+
	lpm r25, Z
	
 ;  0 "" 2
.LVL377:
/* #NOAPP */
.LBE331:
	sts (136)+1,r25
	sts 136,r24
.LM633:
	lds r24,122
.LVL378:
	sbrc r24,6
	rjmp .L414
.L428:
.LM634:
	call adcComplete
.LVL379:
.LM635:
	sbic 35-32,4
	rjmp .L430
.LVL380:
.L415:
.LM636:
	lds r25,flags2
	sbrc r25,6
	rjmp .L416
.LM637:
	ori r25,lo8(64)
	sts flags2,r25
.LM638:
	lds r24,mode
	cpi r24,lo8(1)
	breq .L417
.LM639:
	call startEscapeMode
.LVL381:
	lds r25,flags2
	rjmp .L416
.LVL382:
.L423:
.LM640:
	call randomByte
.LVL383:
	rjmp .L424
.LVL384:
.L429:
.LBB333:
.LBB334:
.LM641:
.LVL385:
	mov r24,r12
.LVL386:
.LBE334:
.LM642:
/* #APP */
 ;  854 "v2c.c" 1
	andi r17, 0x7f
 ;  0 "" 2
.LM643:
/* #NOAPP */
	call touch
.LVL387:
	lds r25,flags2
	rjmp .L422
.LVL388:
.L417:
.LBE333:
.LM644:
	call startEditor
.LVL389:
	lds r25,flags2
	rjmp .L416
.LFE49:
	.size	idleLoop, .-idleLoop
	.section	.debug_frame,"",@progbits
.Lframe0:
	.long	.LECIE0-.LSCIE0
.LSCIE0:
	.long	0xffffffff
	.byte	0x1
	.string	""
	.uleb128 0x1
	.sleb128 -1
	.byte	0x24
	.byte	0xc
	.uleb128 0x20
	.uleb128 0x0
	.p2align	2
.LECIE0:
.LSFDE0:
	.long	.LEFDE0-.LASFDE0
.LASFDE0:
	.long	.Lframe0
	.long	.LFB10
	.long	.LFE10-.LFB10
	.p2align	2
.LEFDE0:
.LSFDE2:
	.long	.LEFDE2-.LASFDE2
.LASFDE2:
	.long	.Lframe0
	.long	.LFB11
	.long	.LFE11-.LFB11
	.p2align	2
.LEFDE2:
.LSFDE4:
	.long	.LEFDE4-.LASFDE4
.LASFDE4:
	.long	.Lframe0
	.long	.LFB12
	.long	.LFE12-.LFB12
	.p2align	2
.LEFDE4:
.LSFDE6:
	.long	.LEFDE6-.LASFDE6
.LASFDE6:
	.long	.Lframe0
	.long	.LFB13
	.long	.LFE13-.LFB13
	.p2align	2
.LEFDE6:
.LSFDE8:
	.long	.LEFDE8-.LASFDE8
.LASFDE8:
	.long	.Lframe0
	.long	.LFB14
	.long	.LFE14-.LFB14
	.p2align	2
.LEFDE8:
.LSFDE10:
	.long	.LEFDE10-.LASFDE10
.LASFDE10:
	.long	.Lframe0
	.long	.LFB15
	.long	.LFE15-.LFB15
	.p2align	2
.LEFDE10:
.LSFDE12:
	.long	.LEFDE12-.LASFDE12
.LASFDE12:
	.long	.Lframe0
	.long	.LFB16
	.long	.LFE16-.LFB16
	.p2align	2
.LEFDE12:
.LSFDE14:
	.long	.LEFDE14-.LASFDE14
.LASFDE14:
	.long	.Lframe0
	.long	.LFB17
	.long	.LFE17-.LFB17
	.p2align	2
.LEFDE14:
.LSFDE16:
	.long	.LEFDE16-.LASFDE16
.LASFDE16:
	.long	.Lframe0
	.long	.LFB18
	.long	.LFE18-.LFB18
	.p2align	2
.LEFDE16:
.LSFDE18:
	.long	.LEFDE18-.LASFDE18
.LASFDE18:
	.long	.Lframe0
	.long	.LFB19
	.long	.LFE19-.LFB19
	.p2align	2
.LEFDE18:
.LSFDE20:
	.long	.LEFDE20-.LASFDE20
.LASFDE20:
	.long	.Lframe0
	.long	.LFB20
	.long	.LFE20-.LFB20
	.p2align	2
.LEFDE20:
.LSFDE22:
	.long	.LEFDE22-.LASFDE22
.LASFDE22:
	.long	.Lframe0
	.long	.LFB21
	.long	.LFE21-.LFB21
	.p2align	2
.LEFDE22:
.LSFDE24:
	.long	.LEFDE24-.LASFDE24
.LASFDE24:
	.long	.Lframe0
	.long	.LFB22
	.long	.LFE22-.LFB22
	.p2align	2
.LEFDE24:
.LSFDE26:
	.long	.LEFDE26-.LASFDE26
.LASFDE26:
	.long	.Lframe0
	.long	.LFB23
	.long	.LFE23-.LFB23
	.p2align	2
.LEFDE26:
.LSFDE28:
	.long	.LEFDE28-.LASFDE28
.LASFDE28:
	.long	.Lframe0
	.long	.LFB24
	.long	.LFE24-.LFB24
	.p2align	2
.LEFDE28:
.LSFDE30:
	.long	.LEFDE30-.LASFDE30
.LASFDE30:
	.long	.Lframe0
	.long	.LFB25
	.long	.LFE25-.LFB25
	.p2align	2
.LEFDE30:
.LSFDE32:
	.long	.LEFDE32-.LASFDE32
.LASFDE32:
	.long	.Lframe0
	.long	.LFB26
	.long	.LFE26-.LFB26
	.p2align	2
.LEFDE32:
.LSFDE34:
	.long	.LEFDE34-.LASFDE34
.LASFDE34:
	.long	.Lframe0
	.long	.LFB27
	.long	.LFE27-.LFB27
	.p2align	2
.LEFDE34:
.LSFDE36:
	.long	.LEFDE36-.LASFDE36
.LASFDE36:
	.long	.Lframe0
	.long	.LFB28
	.long	.LFE28-.LFB28
	.p2align	2
.LEFDE36:
.LSFDE38:
	.long	.LEFDE38-.LASFDE38
.LASFDE38:
	.long	.Lframe0
	.long	.LFB29
	.long	.LFE29-.LFB29
	.p2align	2
.LEFDE38:
.LSFDE40:
	.long	.LEFDE40-.LASFDE40
.LASFDE40:
	.long	.Lframe0
	.long	.LFB30
	.long	.LFE30-.LFB30
	.p2align	2
.LEFDE40:
.LSFDE42:
	.long	.LEFDE42-.LASFDE42
.LASFDE42:
	.long	.Lframe0
	.long	.LFB31
	.long	.LFE31-.LFB31
	.p2align	2
.LEFDE42:
.LSFDE44:
	.long	.LEFDE44-.LASFDE44
.LASFDE44:
	.long	.Lframe0
	.long	.LFB32
	.long	.LFE32-.LFB32
	.p2align	2
.LEFDE44:
.LSFDE46:
	.long	.LEFDE46-.LASFDE46
.LASFDE46:
	.long	.Lframe0
	.long	.LFB33
	.long	.LFE33-.LFB33
	.p2align	2
.LEFDE46:
.LSFDE48:
	.long	.LEFDE48-.LASFDE48
.LASFDE48:
	.long	.Lframe0
	.long	.LFB35
	.long	.LFE35-.LFB35
	.p2align	2
.LEFDE48:
.LSFDE50:
	.long	.LEFDE50-.LASFDE50
.LASFDE50:
	.long	.Lframe0
	.long	.LFB36
	.long	.LFE36-.LFB36
	.p2align	2
.LEFDE50:
.LSFDE52:
	.long	.LEFDE52-.LASFDE52
.LASFDE52:
	.long	.Lframe0
	.long	.LFB37
	.long	.LFE37-.LFB37
	.p2align	2
.LEFDE52:
.LSFDE54:
	.long	.LEFDE54-.LASFDE54
.LASFDE54:
	.long	.Lframe0
	.long	.LFB38
	.long	.LFE38-.LFB38
	.p2align	2
.LEFDE54:
.LSFDE56:
	.long	.LEFDE56-.LASFDE56
.LASFDE56:
	.long	.Lframe0
	.long	.LFB39
	.long	.LFE39-.LFB39
	.p2align	2
.LEFDE56:
.LSFDE58:
	.long	.LEFDE58-.LASFDE58
.LASFDE58:
	.long	.Lframe0
	.long	.LFB40
	.long	.LFE40-.LFB40
	.p2align	2
.LEFDE58:
.LSFDE60:
	.long	.LEFDE60-.LASFDE60
.LASFDE60:
	.long	.Lframe0
	.long	.LFB41
	.long	.LFE41-.LFB41
	.p2align	2
.LEFDE60:
.LSFDE62:
	.long	.LEFDE62-.LASFDE62
.LASFDE62:
	.long	.Lframe0
	.long	.LFB42
	.long	.LFE42-.LFB42
	.p2align	2
.LEFDE62:
.LSFDE64:
	.long	.LEFDE64-.LASFDE64
.LASFDE64:
	.long	.Lframe0
	.long	.LFB43
	.long	.LFE43-.LFB43
	.p2align	2
.LEFDE64:
.LSFDE66:
	.long	.LEFDE66-.LASFDE66
.LASFDE66:
	.long	.Lframe0
	.long	.LFB44
	.long	.LFE44-.LFB44
	.p2align	2
.LEFDE66:
.LSFDE68:
	.long	.LEFDE68-.LASFDE68
.LASFDE68:
	.long	.Lframe0
	.long	.LFB45
	.long	.LFE45-.LFB45
	.p2align	2
.LEFDE68:
.LSFDE70:
	.long	.LEFDE70-.LASFDE70
.LASFDE70:
	.long	.Lframe0
	.long	.LFB46
	.long	.LFE46-.LFB46
	.p2align	2
.LEFDE70:
.LSFDE72:
	.long	.LEFDE72-.LASFDE72
.LASFDE72:
	.long	.Lframe0
	.long	.LFB47
	.long	.LFE47-.LFB47
	.p2align	2
.LEFDE72:
.LSFDE74:
	.long	.LEFDE74-.LASFDE74
.LASFDE74:
	.long	.Lframe0
	.long	.LFB34
	.long	.LFE34-.LFB34
	.p2align	2
.LEFDE74:
.LSFDE76:
	.long	.LEFDE76-.LASFDE76
.LASFDE76:
	.long	.Lframe0
	.long	.LFB48
	.long	.LFE48-.LFB48
	.p2align	2
.LEFDE76:
.LSFDE78:
	.long	.LEFDE78-.LASFDE78
.LASFDE78:
	.long	.Lframe0
	.long	.LFB49
	.long	.LFE49-.LFB49
	.p2align	2
.LEFDE78:
	.text
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST1:
	.long	.LVL0-.Ltext0
	.long	.LVL1-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL2-.Ltext0
	.long	.LVL3-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST3:
	.long	.LVL4-.Ltext0
	.long	.LVL13-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL14-.Ltext0
	.long	.LFE11-.Ltext0
	.word	0x1
	.byte	0x61
	.long	0x0
	.long	0x0
.LLST4:
	.long	.LVL5-.Ltext0
	.long	.LVL10-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	.LVL12-.Ltext0
	.long	.LVL13-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	.LVL14-.Ltext0
	.long	.LFE11-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	0x0
	.long	0x0
.LLST5:
	.long	.LVL6-.Ltext0
	.long	.LVL10-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	.LVL12-.Ltext0
	.long	.LVL13-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	.LVL14-.Ltext0
	.long	.LFE11-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	0x0
	.long	0x0
.LLST6:
	.long	.LVL7-.Ltext0
	.long	.LVL10-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL12-.Ltext0
	.long	.LVL13-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL14-.Ltext0
	.long	.LFE11-.Ltext0
	.word	0x1
	.byte	0x59
	.long	0x0
	.long	0x0
.LLST8:
	.long	.LVL15-.Ltext0
	.long	.LVL16-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL16-.Ltext0
	.long	.LFE12-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST10:
	.long	.LVL17-.Ltext0
	.long	.LVL19-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL22-.Ltext0
	.long	.LVL29-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL30-.Ltext0
	.long	.LVL37-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL38-.Ltext0
	.long	.LVL46-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL47-.Ltext0
	.long	.LFE13-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST11:
	.long	.LVL18-.Ltext0
	.long	.LVL23-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL23-.Ltext0
	.long	.LVL25-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL25-.Ltext0
	.long	.LVL26-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL26-.Ltext0
	.long	.LVL27-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL27-.Ltext0
	.long	.LVL28-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL30-.Ltext0
	.long	.LVL31-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL32-.Ltext0
	.long	.LVL33-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL33-.Ltext0
	.long	.LVL36-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL36-.Ltext0
	.long	.LVL37-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL38-.Ltext0
	.long	.LVL39-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL42-.Ltext0
	.long	.LVL43-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL47-.Ltext0
	.long	.LVL48-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST12:
	.long	.LVL40-.Ltext0
	.long	.LVL41-.Ltext0
	.word	0x1
	.byte	0x5d
	.long	0x0
	.long	0x0
.LLST13:
	.long	.LVL49-.Ltext0
	.long	.LVL50-.Ltext0
	.word	0x1
	.byte	0x5d
	.long	0x0
	.long	0x0
.LLST16:
	.long	.LVL52-.Ltext0
	.long	.LVL53-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST18:
	.long	.LVL54-.Ltext0
	.long	.LVL55-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL55-.Ltext0
	.long	.LFE16-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 5
	.long	0x0
	.long	0x0
.LLST19:
	.long	.LVL56-.Ltext0
	.long	.LVL59-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST20:
	.long	.LVL55-.Ltext0
	.long	.LVL57-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	.LVL58-.Ltext0
	.long	.LFE16-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST22:
	.long	.LVL61-.Ltext0
	.long	.LVL62-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST24:
	.long	.LVL64-.Ltext0
	.long	.LVL65-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL65-.Ltext0
	.long	.LVL68-.Ltext0
	.word	0x1
	.byte	0x6b
	.long	0x0
	.long	0x0
.LLST25:
	.long	.LVL66-.Ltext0
	.long	.LVL69-.Ltext0
	.word	0x6
	.byte	0x6c
	.byte	0x93
	.uleb128 0x1
	.byte	0x6d
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST27:
	.long	.LVL71-.Ltext0
	.long	.LVL74-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST28:
	.long	.LVL70-.Ltext0
	.long	.LVL72-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	.LVL73-.Ltext0
	.long	.LVL76-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST29:
	.long	.LVL77-.Ltext0
	.long	.LVL78-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL81-.Ltext0
	.long	.LVL82-.Ltext0
	.word	0x1
	.byte	0x61
	.long	0x0
	.long	0x0
.LLST30:
	.long	.LVL78-.Ltext0
	.long	.LVL79-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL79-.Ltext0
	.long	.LVL80-.Ltext0
	.word	0x1
	.byte	0x61
	.long	0x0
	.long	0x0
.LLST32:
	.long	.LVL83-.Ltext0
	.long	.LVL92-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 8
	.long	.LVL92-.Ltext0
	.long	.LVL93-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL93-.Ltext0
	.long	.LVL94-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL94-.Ltext0
	.long	.LFE20-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 8
	.long	0x0
	.long	0x0
.LLST33:
	.long	.LVL86-.Ltext0
	.long	.LVL87-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL88-.Ltext0
	.long	.LVL91-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL94-.Ltext0
	.long	.LVL95-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL97-.Ltext0
	.long	.LFE20-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST35:
	.long	.LVL98-.Ltext0
	.long	.LVL104-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 3
	.long	0x0
	.long	0x0
.LLST36:
	.long	.LVL99-.Ltext0
	.long	.LVL102-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST37:
	.long	.LVL98-.Ltext0
	.long	.LVL100-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	.LVL101-.Ltext0
	.long	.LVL104-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST39:
	.long	.LVL105-.Ltext0
	.long	.LVL109-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 3
	.long	.LVL112-.Ltext0
	.long	.LFE22-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 3
	.long	0x0
	.long	0x0
.LLST40:
	.long	.LVL109-.Ltext0
	.long	.LVL111-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST42:
	.long	.LVL113-.Ltext0
	.long	.LVL114-.Ltext0
	.word	0x6
	.byte	0x6c
	.byte	0x93
	.uleb128 0x1
	.byte	0x6d
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST45:
	.long	.LVL116-.Ltext0
	.long	.LVL119-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST46:
	.long	.LVL115-.Ltext0
	.long	.LVL117-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	.LVL118-.Ltext0
	.long	.LVL121-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST48:
	.long	.LVL122-.Ltext0
	.long	.LVL123-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 7
	.long	.LVL124-.Ltext0
	.long	.LVL126-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 7
	.long	0x0
	.long	0x0
.LLST50:
	.long	.LVL127-.Ltext0
	.long	.LVL130-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL130-.Ltext0
	.long	.LFE27-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	0x0
	.long	0x0
.LLST51:
	.long	.LVL128-.Ltext0
	.long	.LVL131-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL131-.Ltext0
	.long	.LVL132-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL132-.Ltext0
	.long	.LVL133-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL133-.Ltext0
	.long	.LVL134-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL134-.Ltext0
	.long	.LVL136-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL136-.Ltext0
	.long	.LVL137-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL138-.Ltext0
	.long	.LFE27-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST53:
	.long	.LVL139-.Ltext0
	.long	.LVL140-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL140-.Ltext0
	.long	.LFE28-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 3
	.long	0x0
	.long	0x0
.LLST54:
	.long	.LVL151-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL154-.Ltext0
	.long	.LVL156-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL158-.Ltext0
	.long	.LVL160-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST55:
	.long	.LVL141-.Ltext0
	.long	.LVL143-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL143-.Ltext0
	.long	.LVL144-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL144-.Ltext0
	.long	.LVL146-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL146-.Ltext0
	.long	.LVL147-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL147-.Ltext0
	.long	.LVL149-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL149-.Ltext0
	.long	.LVL150-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL150-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST56:
	.long	.LVL142-.Ltext0
	.long	.LVL143-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 1
	.long	.LVL143-.Ltext0
	.long	.LVL144-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL144-.Ltext0
	.long	.LVL145-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 1
	.long	.LVL145-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	0x0
	.long	0x0
.LLST57:
	.long	.LVL151-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	.LVL153-.Ltext0
	.long	.LVL155-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	.LVL158-.Ltext0
	.long	.LVL159-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	0x0
	.long	0x0
.LLST58:
	.long	.LVL151-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	.LVL155-.Ltext0
	.long	.LVL158-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	0x0
	.long	0x0
.LLST59:
	.long	.LVL151-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	.LVL157-.Ltext0
	.long	.LVL158-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	0x0
	.long	0x0
.LLST60:
	.long	.LVL151-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	.LVL159-.Ltext0
	.long	.LFE28-.Ltext0
	.word	0x1
	.byte	0x5a
	.long	0x0
	.long	0x0
.LLST61:
	.long	.LVL151-.Ltext0
	.long	.LVL152-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	.LVL161-.Ltext0
	.long	.LFE28-.Ltext0
	.word	0x1
	.byte	0x5f
	.long	0x0
	.long	0x0
.LLST63:
	.long	.LVL162-.Ltext0
	.long	.LVL164-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL164-.Ltext0
	.long	.LVL168-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 1
	.long	.LVL168-.Ltext0
	.long	.LVL169-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL169-.Ltext0
	.long	.LFE29-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST64:
	.long	.LVL163-.Ltext0
	.long	.LVL166-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	0x0
	.long	0x0
.LLST65:
	.long	.LVL165-.Ltext0
	.long	.LVL167-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST67:
	.long	.LVL171-.Ltext0
	.long	.LVL172-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST69:
	.long	.LVL174-.Ltext0
	.long	.LVL175-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL175-.Ltext0
	.long	.LVL176-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL177-.Ltext0
	.long	.LVL178-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	0x0
	.long	0x0
.LLST71:
	.long	.LVL179-.Ltext0
	.long	.LVL180-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL180-.Ltext0
	.long	.LFE32-.Ltext0
	.word	0x1
	.byte	0x6b
	.long	0x0
	.long	0x0
.LLST72:
	.long	.LVL182-.Ltext0
	.long	.LVL183-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL183-.Ltext0
	.long	.LVL186-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL186-.Ltext0
	.long	.LVL187-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL188-.Ltext0
	.long	.LVL190-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL190-.Ltext0
	.long	.LVL191-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL191-.Ltext0
	.long	.LVL194-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL195-.Ltext0
	.long	.LVL197-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL197-.Ltext0
	.long	.LVL201-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL202-.Ltext0
	.long	.LVL203-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	0x0
	.long	0x0
.LLST73:
	.long	.LVL186-.Ltext0
	.long	.LVL189-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL189-.Ltext0
	.long	.LVL190-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL190-.Ltext0
	.long	.LVL191-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL191-.Ltext0
	.long	.LVL192-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL192-.Ltext0
	.long	.LVL193-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL193-.Ltext0
	.long	.LVL194-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL194-.Ltext0
	.long	.LVL196-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL196-.Ltext0
	.long	.LVL200-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL200-.Ltext0
	.long	.LVL202-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL202-.Ltext0
	.long	.LVL203-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL203-.Ltext0
	.long	.LFE32-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	0x0
	.long	0x0
.LLST75:
	.long	.LVL204-.Ltext0
	.long	.LVL205-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL206-.Ltext0
	.long	.LVL207-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL208-.Ltext0
	.long	.LVL209-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL210-.Ltext0
	.long	.LVL211-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL216-.Ltext0
	.long	.LVL217-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST76:
	.long	.LVL211-.Ltext0
	.long	.LVL212-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL212-.Ltext0
	.long	.LVL214-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL215-.Ltext0
	.long	.LVL216-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST77:
	.long	.LVL214-.Ltext0
	.long	.LVL216-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST79:
	.long	.LVL218-.Ltext0
	.long	.LVL219-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST81:
	.long	.LVL224-.Ltext0
	.long	.LVL225-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST82:
	.long	.LVL229-.Ltext0
	.long	.LVL233-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	0x0
	.long	0x0
.LLST83:
	.long	.LVL226-.Ltext0
	.long	.LVL227-.Ltext0
	.word	0x1
	.byte	0x6b
	.long	.LVL232-.Ltext0
	.long	.LFE36-.Ltext0
	.word	0x1
	.byte	0x6b
	.long	0x0
	.long	0x0
.LLST84:
	.long	.LVL233-.Ltext0
	.long	.LVL236-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	0x0
	.long	0x0
.LLST86:
	.long	.LVL237-.Ltext0
	.long	.LVL238-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST88:
	.long	.LVL242-.Ltext0
	.long	.LVL246-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST90:
	.long	.LVL247-.Ltext0
	.long	.LVL249-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST93:
	.long	.LVL250-.Ltext0
	.long	.LVL251-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL252-.Ltext0
	.long	.LVL253-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL254-.Ltext0
	.long	.LVL255-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL256-.Ltext0
	.long	.LVL257-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL258-.Ltext0
	.long	.LVL259-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL260-.Ltext0
	.long	.LVL261-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST95:
	.long	.LVL262-.Ltext0
	.long	.LVL263-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST96:
	.long	.LVL266-.Ltext0
	.long	.LVL267-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST97:
	.long	.LVL268-.Ltext0
	.long	.LVL269-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST99:
	.long	.LVL270-.Ltext0
	.long	.LVL271-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST100:
	.long	.LVL273-.Ltext0
	.long	.LVL275-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST101:
	.long	.LVL272-.Ltext0
	.long	.LVL274-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL276-.Ltext0
	.long	.LFE43-.Ltext0
	.word	0x1
	.byte	0x69
	.long	0x0
	.long	0x0
.LLST102:
	.long	.LVL277-.Ltext0
	.long	.LVL278-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST104:
	.long	.LVL279-.Ltext0
	.long	.LVL280-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST105:
	.long	.LVL282-.Ltext0
	.long	.LVL284-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	.LVL284-.Ltext0
	.long	.LVL285-.Ltext0
	.word	0x1
	.byte	0x6a
	.long	.LVL285-.Ltext0
	.long	.LVL292-.Ltext0
	.word	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	.LVL297-.Ltext0
	.long	.LVL298-.Ltext0
	.word	0x7
	.byte	0x8c
	.sleb128 2
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.long	.LVL298-.Ltext0
	.long	.LFE44-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 2
	.long	0x0
	.long	0x0
.LLST106:
	.long	.LVL282-.Ltext0
	.long	.LVL287-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 1
	.long	.LVL287-.Ltext0
	.long	.LVL288-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL288-.Ltext0
	.long	.LVL291-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL293-.Ltext0
	.long	.LVL295-.Ltext0
	.word	0x7
	.byte	0x8c
	.sleb128 1
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL295-.Ltext0
	.long	.LFE44-.Ltext0
	.word	0x2
	.byte	0x8c
	.sleb128 1
	.long	0x0
	.long	0x0
.LLST107:
	.long	.LVL291-.Ltext0
	.long	.LVL294-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST108:
	.long	.LVL282-.Ltext0
	.long	.LVL283-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL296-.Ltext0
	.long	.LFE44-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST110:
	.long	.LVL299-.Ltext0
	.long	.LVL302-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST111:
	.long	.LVL301-.Ltext0
	.long	.LVL304-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL304-.Ltext0
	.long	.LVL307-.Ltext0
	.word	0x7
	.byte	0x8c
	.sleb128 3
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL307-.Ltext0
	.long	.LFE45-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST112:
	.long	.LVL305-.Ltext0
	.long	.LVL306-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL306-.Ltext0
	.long	.LVL309-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST114:
	.long	.LVL310-.Ltext0
	.long	.LVL315-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST115:
	.long	.LVL314-.Ltext0
	.long	.LVL318-.Ltext0
	.word	0x1
	.byte	0x69
	.long	.LVL318-.Ltext0
	.long	.LVL320-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST116:
	.long	.LVL315-.Ltext0
	.long	.LVL316-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL316-.Ltext0
	.long	.LVL317-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL317-.Ltext0
	.long	.LFE46-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST117:
	.long	.LVL320-.Ltext0
	.long	.LVL321-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST118:
	.long	.LVL321-.Ltext0
	.long	.LVL322-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST119:
	.long	.LVL322-.Ltext0
	.long	.LVL323-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST120:
	.long	.LVL323-.Ltext0
	.long	.LVL325-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST122:
	.long	.LVL326-.Ltext0
	.long	.LVL327-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL328-.Ltext0
	.long	.LVL329-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL330-.Ltext0
	.long	.LVL331-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL332-.Ltext0
	.long	.LVL333-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL334-.Ltext0
	.long	.LVL335-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL336-.Ltext0
	.long	.LVL337-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST124:
	.long	.LVL338-.Ltext0
	.long	.LVL339-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST125:
	.long	.LVL340-.Ltext0
	.long	.LVL341-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL346-.Ltext0
	.long	.LVL347-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST126:
	.long	.LVL340-.Ltext0
	.long	.LVL341-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL342-.Ltext0
	.long	.LVL343-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL349-.Ltext0
	.long	.LVL350-.Ltext0
	.word	0x1
	.byte	0x61
	.long	0x0
	.long	0x0
.LLST127:
	.long	.LVL340-.Ltext0
	.long	.LVL341-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL344-.Ltext0
	.long	.LVL345-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL348-.Ltext0
	.long	.LVL349-.Ltext0
	.word	0x1
	.byte	0x61
	.long	0x0
	.long	0x0
.LLST128:
	.long	.LVL340-.Ltext0
	.long	.LVL341-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	.LVL351-.Ltext0
	.long	.LFE34-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST129:
	.long	.LVL340-.Ltext0
	.long	.LVL341-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	.LVL352-.Ltext0
	.long	.LFE34-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST131:
	.long	.LVL353-.Ltext0
	.long	.LVL354-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL355-.Ltext0
	.long	.LVL356-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL357-.Ltext0
	.long	.LVL358-.Ltext0
	.word	0x1
	.byte	0x68
	.long	.LVL359-.Ltext0
	.long	.LVL360-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST133:
	.long	.LVL362-.Ltext0
	.long	.LVL367-.Ltext0
	.word	0x1
	.byte	0x5d
	.long	.LVL382-.Ltext0
	.long	.LVL383-.Ltext0
	.word	0x1
	.byte	0x5d
	.long	.LVL384-.Ltext0
	.long	.LVL387-.Ltext0
	.word	0x1
	.byte	0x5d
	.long	0x0
	.long	0x0
.LLST134:
	.long	.LVL366-.Ltext0
	.long	.LVL367-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL382-.Ltext0
	.long	.LVL383-.Ltext0
	.word	0x1
	.byte	0x61
	.long	.LVL384-.Ltext0
	.long	.LVL387-.Ltext0
	.word	0x1
	.byte	0x61
	.long	0x0
	.long	0x0
.LLST135:
	.long	.LVL369-.Ltext0
	.long	.LVL371-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST136:
	.long	.LVL368-.Ltext0
	.long	.LVL370-.Ltext0
	.word	0x1
	.byte	0x59
	.long	0x0
	.long	0x0
.LLST137:
	.long	.LVL372-.Ltext0
	.long	.LVL374-.Ltext0
	.word	0x1
	.byte	0x6e
	.long	0x0
	.long	0x0
.LLST138:
	.long	.LVL370-.Ltext0
	.long	.LVL373-.Ltext0
	.word	0x1
	.byte	0x59
	.long	0x0
	.long	0x0
.LLST139:
	.long	.LVL361-.Ltext0
	.long	.LVL363-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL365-.Ltext0
	.long	.LVL367-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL375-.Ltext0
	.long	.LVL379-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL380-.Ltext0
	.long	.LVL381-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL382-.Ltext0
	.long	.LVL383-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL384-.Ltext0
	.long	.LVL387-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	.LVL388-.Ltext0
	.long	.LVL389-.Ltext0
	.word	0x6
	.byte	0x6e
	.byte	0x93
	.uleb128 0x1
	.byte	0x6f
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST140:
	.long	.LVL377-.Ltext0
	.long	.LVL378-.Ltext0
	.word	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.long	0x0
	.long	0x0
.LLST141:
	.long	.LVL361-.Ltext0
	.long	.LVL367-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL373-.Ltext0
	.long	.LVL379-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL380-.Ltext0
	.long	.LVL381-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL382-.Ltext0
	.long	.LVL383-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL384-.Ltext0
	.long	.LVL387-.Ltext0
	.word	0x1
	.byte	0x59
	.long	.LVL388-.Ltext0
	.long	.LVL389-.Ltext0
	.word	0x1
	.byte	0x59
	.long	0x0
	.long	0x0
.LLST142:
	.long	.LVL386-.Ltext0
	.long	.LVL387-.Ltext0
	.word	0x1
	.byte	0x68
	.long	0x0
	.long	0x0
.LLST143:
	.long	.LVL385-.Ltext0
	.long	.LVL387-.Ltext0
	.word	0x1
	.byte	0x5c
	.long	0x0
	.long	0x0
	.section	.debug_info
	.long	0x1b60
	.word	0x2
	.long	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.long	.LASF130
	.byte	0x1
	.long	.LASF131
	.long	.LASF132
	.long	.Ltext0
	.long	.Letext0
	.long	.Ldebug_line0
	.uleb128 0x2
	.long	.LASF0
	.byte	0x3
	.byte	0x79
	.long	0x30
	.uleb128 0x3
	.byte	0x1
	.byte	0x6
	.long	.LASF2
	.uleb128 0x2
	.long	.LASF1
	.byte	0x3
	.byte	0x7a
	.long	0x42
	.uleb128 0x3
	.byte	0x1
	.byte	0x8
	.long	.LASF3
	.uleb128 0x2
	.long	.LASF4
	.byte	0x3
	.byte	0x7b
	.long	0x54
	.uleb128 0x4
	.byte	0x2
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.long	.LASF5
	.byte	0x3
	.byte	0x7c
	.long	0x66
	.uleb128 0x3
	.byte	0x2
	.byte	0x7
	.long	.LASF6
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.long	.LASF7
	.uleb128 0x3
	.byte	0x4
	.byte	0x7
	.long	.LASF8
	.uleb128 0x3
	.byte	0x8
	.byte	0x5
	.long	.LASF9
	.uleb128 0x3
	.byte	0x8
	.byte	0x7
	.long	.LASF10
	.uleb128 0x5
	.byte	0x1
	.byte	0x6
	.uleb128 0x5
	.byte	0x1
	.byte	0x8
	.uleb128 0x2
	.long	.LASF11
	.byte	0x1
	.byte	0x6
	.long	0x37
	.uleb128 0x6
	.byte	0x1
	.byte	0x1
	.byte	0xb
	.long	0x12b
	.uleb128 0x7
	.long	.LASF12
	.byte	0x1
	.byte	0xc
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x7
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF13
	.byte	0x1
	.byte	0xd
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x6
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF14
	.byte	0x1
	.byte	0xe
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x5
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF15
	.byte	0x1
	.byte	0xf
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x4
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF16
	.byte	0x1
	.byte	0x10
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x3
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF17
	.byte	0x1
	.byte	0x11
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x2
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF18
	.byte	0x1
	.byte	0x12
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.uleb128 0x7
	.long	.LASF19
	.byte	0x1
	.byte	0x13
	.long	0x8f
	.byte	0x1
	.byte	0x1
	.byte	0x0
	.byte	0x2
	.byte	0x23
	.uleb128 0x0
	.byte	0x0
	.uleb128 0x2
	.long	.LASF20
	.byte	0x1
	.byte	0x14
	.long	0x9a
	.uleb128 0x8
	.byte	0x1
	.long	.LASF21
	.byte	0x1
	.byte	0x8a
	.byte	0x1
	.long	0x37
	.byte	0x0
	.long	0x154
	.uleb128 0x9
	.long	.LASF23
	.byte	0x1
	.byte	0x89
	.long	0x37
	.byte	0x0
	.uleb128 0x8
	.byte	0x1
	.long	.LASF22
	.byte	0x1
	.byte	0xb8
	.byte	0x1
	.long	0x37
	.byte	0x0
	.long	0x172
	.uleb128 0x9
	.long	.LASF24
	.byte	0x1
	.byte	0xb7
	.long	0x37
	.byte	0x0
	.uleb128 0xa
	.byte	0x1
	.long	.LASF35
	.byte	0x1
	.word	0x126
	.byte	0x1
	.byte	0x0
	.long	0x1ca
	.uleb128 0xb
	.long	.LASF25
	.byte	0x1
	.word	0x125
	.long	0x1ca
	.uleb128 0xc
	.uleb128 0xd
	.long	.LASF26
	.byte	0x1
	.word	0x127
	.long	0x54
	.uleb128 0xc
	.uleb128 0xe
	.long	0x1b9
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x128
	.long	0x5b
	.uleb128 0xd
	.long	.LASF28
	.byte	0x1
	.word	0x128
	.long	0x37
	.byte	0x0
	.uleb128 0xc
	.uleb128 0xd
	.long	.LASF29
	.byte	0x1
	.word	0x129
	.long	0x54
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0xf
	.byte	0x2
	.long	0x37
	.uleb128 0x10
	.long	.LASF133
	.byte	0x2
	.byte	0xc4
	.byte	0x1
	.long	0x37
	.byte	0x3
	.long	0x1f8
	.uleb128 0x11
	.string	"__p"
	.byte	0x2
	.byte	0xc3
	.long	0x1f8
	.uleb128 0x12
	.long	.LASF28
	.byte	0x2
	.byte	0xcf
	.long	0x37
	.byte	0x0
	.uleb128 0xf
	.byte	0x2
	.long	0x1fe
	.uleb128 0x13
	.long	0x37
	.uleb128 0x14
	.byte	0x1
	.long	.LASF30
	.byte	0x1
	.word	0x13a
	.byte	0x1
	.long	0x37
	.byte	0x0
	.long	0x23c
	.uleb128 0xb
	.long	.LASF31
	.byte	0x1
	.word	0x139
	.long	0x37
	.uleb128 0xd
	.long	.LASF32
	.byte	0x1
	.word	0x13c
	.long	0x49
	.uleb128 0xc
	.uleb128 0x15
	.string	"xx"
	.byte	0x1
	.word	0x13d
	.long	0x37
	.byte	0x0
	.byte	0x0
	.uleb128 0x16
	.byte	0x1
	.long	.LASF33
	.byte	0x1
	.word	0x17a
	.byte	0x0
	.long	0x257
	.uleb128 0xc
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x17b
	.long	0x54
	.byte	0x0
	.byte	0x0
	.uleb128 0x16
	.byte	0x1
	.long	.LASF34
	.byte	0x1
	.word	0x180
	.byte	0x0
	.long	0x27a
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x181
	.long	0x37
	.uleb128 0x15
	.string	"y"
	.byte	0x1
	.word	0x181
	.long	0x37
	.byte	0x0
	.uleb128 0xa
	.byte	0x1
	.long	.LASF36
	.byte	0x1
	.word	0x1a4
	.byte	0x1
	.byte	0x0
	.long	0x2bc
	.uleb128 0xb
	.long	.LASF37
	.byte	0x1
	.word	0x1a3
	.long	0x37
	.uleb128 0xd
	.long	.LASF38
	.byte	0x1
	.word	0x1a5
	.long	0x37
	.uleb128 0xd
	.long	.LASF39
	.byte	0x1
	.word	0x1a6
	.long	0x37
	.uleb128 0xc
	.uleb128 0xd
	.long	.LASF40
	.byte	0x1
	.word	0x1a7
	.long	0x37
	.byte	0x0
	.byte	0x0
	.uleb128 0xa
	.byte	0x1
	.long	.LASF41
	.byte	0x1
	.word	0x1de
	.byte	0x1
	.byte	0x0
	.long	0x2fe
	.uleb128 0xb
	.long	.LASF37
	.byte	0x1
	.word	0x1dd
	.long	0x37
	.uleb128 0xd
	.long	.LASF42
	.byte	0x1
	.word	0x1df
	.long	0x37
	.uleb128 0xc
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x1e0
	.long	0x5b
	.uleb128 0xd
	.long	.LASF28
	.byte	0x1
	.word	0x1e0
	.long	0x37
	.byte	0x0
	.byte	0x0
	.uleb128 0x17
	.long	.LASF134
	.byte	0x2
	.word	0x12b
	.byte	0x1
	.byte	0x3
	.long	0x325
	.uleb128 0x18
	.string	"__p"
	.byte	0x2
	.word	0x12a
	.long	0x1ca
	.uleb128 0xb
	.long	.LASF43
	.byte	0x2
	.word	0x12a
	.long	0x37
	.byte	0x0
	.uleb128 0x16
	.byte	0x1
	.long	.LASF44
	.byte	0x1
	.word	0x120
	.byte	0x0
	.long	0x35a
	.uleb128 0xc
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x121
	.long	0x54
	.uleb128 0xc
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x122
	.long	0x5b
	.uleb128 0xd
	.long	.LASF28
	.byte	0x1
	.word	0x122
	.long	0x37
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x136
	.long	.LFB10
	.long	.LFE10
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x378
	.uleb128 0x1a
	.long	0x148
	.long	.LLST1
	.byte	0x0
	.uleb128 0x1b
	.byte	0x1
	.long	.LASF45
	.byte	0x1
	.byte	0x91
	.long	.LFB11
	.long	.LFE11
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x43f
	.uleb128 0x1c
	.long	0x136
	.long	.Ldebug_ranges0+0x0
	.byte	0x1
	.byte	0x9a
	.long	0x3a4
	.uleb128 0x1d
	.long	0x36e
	.byte	0x0
	.uleb128 0x1c
	.long	0x136
	.long	.Ldebug_ranges0+0x18
	.byte	0x1
	.byte	0x96
	.long	0x3b9
	.uleb128 0x1d
	.long	0x36e
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB151
	.long	.LBE151
	.long	0x3d6
	.uleb128 0x1f
	.string	"val"
	.byte	0x1
	.byte	0x9e
	.long	0x12b
	.long	.LLST3
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB152
	.long	.LBE152
	.long	0x3f3
	.uleb128 0x1f
	.string	"val"
	.byte	0x1
	.byte	0x9f
	.long	0x37
	.long	.LLST4
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB153
	.long	.LBE153
	.long	0x410
	.uleb128 0x1f
	.string	"val"
	.byte	0x1
	.byte	0xa0
	.long	0x37
	.long	.LLST5
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB154
	.long	.LBE154
	.long	0x42d
	.uleb128 0x1f
	.string	"val"
	.byte	0x1
	.byte	0xa1
	.long	0x37
	.long	.LLST6
	.byte	0x0
	.uleb128 0x20
	.long	0x136
	.long	.Ldebug_ranges0+0x30
	.byte	0x1
	.byte	0xad
	.uleb128 0x1d
	.long	0x36e
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x154
	.long	.LFB12
	.long	.LFE12
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x45d
	.uleb128 0x1a
	.long	0x166
	.long	.LLST8
	.byte	0x0
	.uleb128 0x21
	.byte	0x1
	.long	.LASF46
	.byte	0x1
	.byte	0xc3
	.byte	0x1
	.long	.LFB13
	.long	.LFE13
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x4df
	.uleb128 0x22
	.long	.LASF24
	.byte	0x1
	.byte	0xc4
	.long	0x37
	.long	.LLST10
	.uleb128 0x22
	.long	.LASF23
	.byte	0x1
	.byte	0xc5
	.long	0x37
	.long	.LLST11
	.uleb128 0x1c
	.long	0x154
	.long	.Ldebug_ranges0+0x48
	.byte	0x1
	.byte	0xc5
	.long	0x4a8
	.uleb128 0x1d
	.long	0x453
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB167
	.long	.LBE167
	.long	0x4c5
	.uleb128 0x1f
	.string	"val"
	.byte	0x1
	.byte	0xf1
	.long	0x37
	.long	.LLST12
	.byte	0x0
	.uleb128 0x23
	.long	.LBB168
	.long	.LBE168
	.uleb128 0x1f
	.string	"val"
	.byte	0x1
	.byte	0xe7
	.long	0x37
	.long	.LLST13
	.byte	0x0
	.byte	0x0
	.uleb128 0x24
	.byte	0x1
	.long	.LASF135
	.byte	0x1
	.word	0x11a
	.byte	0x1
	.long	.LFB14
	.long	.LFE14
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.uleb128 0x19
	.long	0x325
	.long	.LFB15
	.long	.LFE15
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x537
	.uleb128 0x23
	.long	.LBB169
	.long	.LBE169
	.uleb128 0x25
	.long	0x334
	.byte	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.uleb128 0x23
	.long	.LBB170
	.long	.LBE170
	.uleb128 0x26
	.long	0x33f
	.uleb128 0x27
	.long	0x34b
	.long	.LLST16
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x172
	.long	.LFB16
	.long	.LFE16
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x5a0
	.uleb128 0x1a
	.long	0x181
	.long	.LLST18
	.uleb128 0x23
	.long	.LBB171
	.long	.LBE171
	.uleb128 0x25
	.long	0x18e
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x23
	.long	.LBB172
	.long	.LBE172
	.uleb128 0x1e
	.long	.LBB173
	.long	.LBE173
	.long	0x58a
	.uleb128 0x26
	.long	0x1a0
	.uleb128 0x27
	.long	0x1ac
	.long	.LLST19
	.byte	0x0
	.uleb128 0x23
	.long	.LBB174
	.long	.LBE174
	.uleb128 0x27
	.long	0x1ba
	.long	.LLST20
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF47
	.byte	0x1
	.word	0x12f
	.byte	0x1
	.long	.LFB17
	.long	.LFE17
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x616
	.uleb128 0x29
	.long	.LASF56
	.byte	0x1
	.word	0x12e
	.long	0x37
	.long	.LLST22
	.uleb128 0xd
	.long	.LASF48
	.byte	0x1
	.word	0x130
	.long	0x37
	.uleb128 0x23
	.long	.LBB175
	.long	.LBE175
	.uleb128 0x15
	.string	"y"
	.byte	0x1
	.word	0x131
	.long	0x37
	.uleb128 0x23
	.long	.LBB176
	.long	.LBE176
	.uleb128 0x2a
	.string	"x"
	.byte	0x1
	.word	0x132
	.long	0x37
	.byte	0x1
	.byte	0x69
	.uleb128 0x23
	.long	.LBB177
	.long	.LBE177
	.uleb128 0x15
	.string	"led"
	.byte	0x1
	.word	0x133
	.long	0x37
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x203
	.long	.LFB18
	.long	.LFE18
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x64e
	.uleb128 0x1a
	.long	0x216
	.long	.LLST24
	.uleb128 0x27
	.long	0x222
	.long	.LLST25
	.uleb128 0x23
	.long	.LBB178
	.long	.LBE178
	.uleb128 0x25
	.long	0x22f
	.byte	0x1
	.byte	0x6a
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF49
	.byte	0x1
	.word	0x143
	.long	.LFB19
	.long	.LFE19
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x704
	.uleb128 0x2c
	.long	0x172
	.long	.LBB179
	.long	.LBE179
	.byte	0x1
	.word	0x144
	.long	0x6cb
	.uleb128 0x1d
	.long	0x54b
	.uleb128 0x23
	.long	.LBB181
	.long	.LBE181
	.uleb128 0x25
	.long	0x18e
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x23
	.long	.LBB182
	.long	.LBE182
	.uleb128 0x1e
	.long	.LBB183
	.long	.LBE183
	.long	0x6b5
	.uleb128 0x26
	.long	0x1a0
	.uleb128 0x27
	.long	0x1ac
	.long	.LLST27
	.byte	0x0
	.uleb128 0x23
	.long	.LBB184
	.long	.LBE184
	.uleb128 0x27
	.long	0x1ba
	.long	.LLST28
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB185
	.long	.LBE185
	.long	0x6e9
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x147
	.long	0x12b
	.long	.LLST29
	.byte	0x0
	.uleb128 0x23
	.long	.LBB186
	.long	.LBE186
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x149
	.long	0x12b
	.long	.LLST30
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF50
	.byte	0x1
	.word	0x150
	.long	.LFB20
	.long	.LFE20
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x7a7
	.uleb128 0x2e
	.long	.Ldebug_ranges0+0x68
	.uleb128 0x2a
	.string	"x"
	.byte	0x1
	.word	0x151
	.long	0x54
	.byte	0x2
	.byte	0x8c
	.sleb128 4
	.uleb128 0x2e
	.long	.Ldebug_ranges0+0x80
	.uleb128 0x2a
	.string	"on"
	.byte	0x1
	.word	0x152
	.long	0x8f
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x2f
	.long	.LASF32
	.byte	0x1
	.word	0x153
	.long	0x5b
	.long	.LLST32
	.uleb128 0x23
	.long	.LBB189
	.long	.LBE189
	.uleb128 0x15
	.string	"y"
	.byte	0x1
	.word	0x154
	.long	0x37
	.uleb128 0x23
	.long	.LBB190
	.long	.LBE190
	.uleb128 0x2f
	.long	.LASF26
	.byte	0x1
	.word	0x155
	.long	0x37
	.long	.LLST33
	.uleb128 0x30
	.long	0x1d0
	.long	.LBB191
	.long	.LBE191
	.byte	0x1
	.word	0x155
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB192
	.long	.LBE192
	.uleb128 0x26
	.long	0x1ec
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF51
	.byte	0x1
	.word	0x167
	.long	.LFB21
	.long	.LFE21
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x822
	.uleb128 0x30
	.long	0x172
	.long	.LBB195
	.long	.LBE195
	.byte	0x1
	.word	0x16a
	.uleb128 0x1d
	.long	0x54b
	.uleb128 0x23
	.long	.LBB197
	.long	.LBE197
	.uleb128 0x27
	.long	0x18e
	.long	.LLST35
	.uleb128 0x23
	.long	.LBB198
	.long	.LBE198
	.uleb128 0x1e
	.long	.LBB199
	.long	.LBE199
	.long	0x80b
	.uleb128 0x26
	.long	0x1a0
	.uleb128 0x27
	.long	0x1ac
	.long	.LLST36
	.byte	0x0
	.uleb128 0x23
	.long	.LBB200
	.long	.LBE200
	.uleb128 0x27
	.long	0x1ba
	.long	.LLST37
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF52
	.byte	0x1
	.word	0x171
	.long	.LFB22
	.long	.LFE22
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x887
	.uleb128 0x2d
	.string	"x"
	.byte	0x1
	.word	0x172
	.long	0x54
	.long	.LLST39
	.uleb128 0x30
	.long	0x203
	.long	.LBB201
	.long	.LBE201
	.byte	0x1
	.word	0x176
	.uleb128 0x31
	.long	0x62a
	.byte	0x2
	.byte	0x8c
	.sleb128 2
	.uleb128 0x23
	.long	.LBB202
	.long	.LBE202
	.uleb128 0x27
	.long	0x222
	.long	.LLST40
	.uleb128 0x23
	.long	.LBB203
	.long	.LBE203
	.uleb128 0x25
	.long	0x22f
	.byte	0x2
	.byte	0x8c
	.sleb128 1
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x23c
	.long	.LFB23
	.long	.LFE23
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x8af
	.uleb128 0x23
	.long	.LBB204
	.long	.LBE204
	.uleb128 0x27
	.long	0x24b
	.long	.LLST42
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x257
	.long	.LFB24
	.long	.LFE24
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x8ce
	.uleb128 0x26
	.long	0x265
	.uleb128 0x26
	.long	0x26f
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF53
	.byte	0x1
	.word	0x188
	.long	.LFB25
	.long	.LFE25
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x952
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x189
	.long	0x37
	.uleb128 0x30
	.long	0x172
	.long	.LBB205
	.long	.LBE205
	.byte	0x1
	.word	0x18a
	.uleb128 0x1d
	.long	0x54b
	.uleb128 0x23
	.long	.LBB207
	.long	.LBE207
	.uleb128 0x25
	.long	0x18e
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x23
	.long	.LBB208
	.long	.LBE208
	.uleb128 0x1e
	.long	.LBB209
	.long	.LBE209
	.long	0x93b
	.uleb128 0x26
	.long	0x1a0
	.uleb128 0x27
	.long	0x1ac
	.long	.LLST45
	.byte	0x0
	.uleb128 0x23
	.long	.LBB210
	.long	.LBE210
	.uleb128 0x27
	.long	0x1ba
	.long	.LLST46
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF54
	.byte	0x1
	.word	0x198
	.long	.LFB26
	.long	.LFE26
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x9b8
	.uleb128 0x2c
	.long	0x23c
	.long	.LBB211
	.long	.LBE211
	.byte	0x1
	.word	0x19d
	.long	0x992
	.uleb128 0x23
	.long	.LBB213
	.long	.LBE213
	.uleb128 0x27
	.long	0x24b
	.long	.LLST48
	.byte	0x0
	.byte	0x0
	.uleb128 0x30
	.long	0x257
	.long	.LBB214
	.long	.LBE214
	.byte	0x1
	.word	0x19e
	.uleb128 0x23
	.long	.LBB215
	.long	.LBE215
	.uleb128 0x26
	.long	0x265
	.uleb128 0x26
	.long	0x26f
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x27a
	.long	.LFB27
	.long	.LFE27
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x9f2
	.uleb128 0x1a
	.long	0x289
	.long	.LLST50
	.uleb128 0x27
	.long	0x295
	.long	.LLST51
	.uleb128 0x25
	.long	0x2a1
	.byte	0x2
	.byte	0x8c
	.sleb128 1
	.uleb128 0x2e
	.long	.Ldebug_ranges0+0x98
	.uleb128 0x26
	.long	0x2ae
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF55
	.byte	0x1
	.word	0x1b6
	.byte	0x1
	.long	.LFB28
	.long	.LFE28
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xaf7
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x1b5
	.long	0x37
	.long	.LLST53
	.uleb128 0x2f
	.long	.LASF57
	.byte	0x1
	.word	0x1bc
	.long	0x37
	.long	.LLST54
	.uleb128 0x32
	.long	0x27a
	.long	.Ldebug_ranges0+0xb0
	.byte	0x1
	.word	0x1b8
	.long	0xa64
	.uleb128 0x1d
	.long	0x9cc
	.uleb128 0x2e
	.long	.Ldebug_ranges0+0xc8
	.uleb128 0x27
	.long	0x295
	.long	.LLST55
	.uleb128 0x27
	.long	0x2a1
	.long	.LLST56
	.uleb128 0x2e
	.long	.Ldebug_ranges0+0xe0
	.uleb128 0x26
	.long	0x2ae
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB224
	.long	.LBE224
	.long	0xa82
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x1bc
	.long	0x37
	.long	.LLST57
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB225
	.long	.LBE225
	.long	0xaa0
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x1c3
	.long	0x37
	.long	.LLST58
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB226
	.long	.LBE226
	.long	0xabe
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x1c5
	.long	0x37
	.long	.LLST59
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB227
	.long	.LBE227
	.long	0xadc
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x1ca
	.long	0x37
	.long	.LLST60
	.byte	0x0
	.uleb128 0x23
	.long	.LBB228
	.long	.LBE228
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x1cc
	.long	0x37
	.long	.LLST61
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF58
	.byte	0x1
	.word	0x1d2
	.byte	0x1
	.long	.LFB29
	.long	.LFE29
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xb93
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x1d1
	.long	0x37
	.long	.LLST63
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x1d4
	.long	0x37
	.uleb128 0x2f
	.long	.LASF42
	.byte	0x1
	.word	0x1d5
	.long	0x25
	.long	.LLST64
	.uleb128 0x1e
	.long	.LBB229
	.long	.LBE229
	.long	0xb60
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x1d5
	.long	0x5b
	.uleb128 0xd
	.long	.LASF28
	.byte	0x1
	.word	0x1d5
	.long	0x37
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB230
	.long	.LBE230
	.long	0xb7e
	.uleb128 0x2f
	.long	.LASF59
	.byte	0x1
	.word	0x1d6
	.long	0x37
	.long	.LLST65
	.byte	0x0
	.uleb128 0x23
	.long	.LBB231
	.long	.LBE231
	.uleb128 0x15
	.string	"y"
	.byte	0x1
	.word	0x1d8
	.long	0x37
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.long	0x2bc
	.long	.LFB30
	.long	.LFE30
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xbce
	.uleb128 0x1a
	.long	0x2cb
	.long	.LLST67
	.uleb128 0x25
	.long	0x2d7
	.byte	0x1
	.byte	0x68
	.uleb128 0x23
	.long	.LBB232
	.long	.LBE232
	.uleb128 0x26
	.long	0x2e4
	.uleb128 0x25
	.long	0x2f0
	.byte	0x1
	.byte	0x6e
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF60
	.byte	0x1
	.word	0x1e5
	.byte	0x1
	.long	.LFB31
	.long	.LFE31
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xbf8
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x1e4
	.long	0x37
	.long	.LLST69
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF61
	.byte	0x1
	.word	0x1ee
	.byte	0x1
	.long	.LFB32
	.long	.LFE32
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xc4b
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x1ed
	.long	0x37
	.long	.LLST71
	.uleb128 0x2a
	.string	"x"
	.byte	0x1
	.word	0x1ef
	.long	0x37
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x2d
	.string	"y"
	.byte	0x1
	.word	0x1f0
	.long	0x37
	.long	.LLST72
	.uleb128 0x2d
	.string	"z"
	.byte	0x1
	.word	0x1f1
	.long	0x37
	.long	.LLST73
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF62
	.byte	0x1
	.word	0x22b
	.byte	0x1
	.long	.LFB33
	.long	.LFE33
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xcb6
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x22a
	.long	0x37
	.long	.LLST75
	.uleb128 0x30
	.long	0x2bc
	.long	.LBB233
	.long	.LBE233
	.byte	0x1
	.word	0x22f
	.uleb128 0x1d
	.long	0xba7
	.uleb128 0x23
	.long	.LBB234
	.long	.LBE234
	.uleb128 0x27
	.long	0x2d7
	.long	.LLST76
	.uleb128 0x23
	.long	.LBB235
	.long	.LBE235
	.uleb128 0x26
	.long	0x2e4
	.uleb128 0x27
	.long	0x2f0
	.long	.LLST77
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF63
	.byte	0x1
	.word	0x28e
	.byte	0x1
	.long	.LFB35
	.long	.LFE35
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xd63
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x28d
	.long	0x37
	.long	.LLST79
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x290
	.long	0x1ca
	.byte	0x2
	.byte	0x8c
	.sleb128 4
	.uleb128 0x23
	.long	.LBB236
	.long	.LBE236
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x291
	.long	0x37
	.uleb128 0x23
	.long	.LBB237
	.long	.LBE237
	.uleb128 0x2a
	.string	"x"
	.byte	0x1
	.word	0x292
	.long	0x37
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x1e
	.long	.LBB238
	.long	.LBE238
	.long	0xd45
	.uleb128 0x15
	.string	"y"
	.byte	0x1
	.word	0x293
	.long	0x37
	.uleb128 0x23
	.long	.LBB239
	.long	.LBE239
	.uleb128 0x2a
	.string	"val"
	.byte	0x1
	.word	0x294
	.long	0x37
	.byte	0x1
	.byte	0x5a
	.byte	0x0
	.byte	0x0
	.uleb128 0x30
	.long	0x2fe
	.long	.LBB240
	.long	.LBE240
	.byte	0x1
	.word	0x296
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF64
	.byte	0x1
	.word	0x29b
	.byte	0x1
	.long	.LFB36
	.long	.LFE36
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xe5b
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x29a
	.long	0x37
	.long	.LLST81
	.uleb128 0x15
	.string	"p"
	.byte	0x1
	.word	0x29d
	.long	0x1ca
	.uleb128 0x23
	.long	.LBB242
	.long	.LBE242
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x29e
	.long	0x37
	.uleb128 0x23
	.long	.LBB243
	.long	.LBE243
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x29f
	.long	0x37
	.uleb128 0x2c
	.long	0x203
	.long	.LBB244
	.long	.LBE244
	.byte	0x1
	.word	0x29f
	.long	0xdfe
	.uleb128 0x31
	.long	0x62a
	.byte	0x2
	.byte	0x8c
	.sleb128 9
	.uleb128 0x23
	.long	.LBB245
	.long	.LBE245
	.uleb128 0x25
	.long	0x222
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x23
	.long	.LBB246
	.long	.LBE246
	.uleb128 0x27
	.long	0x22f
	.long	.LLST82
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.long	0x203
	.long	.LBB247
	.long	.LBE247
	.byte	0x1
	.word	0x29f
	.long	0xe41
	.uleb128 0x1a
	.long	0x62a
	.long	.LLST83
	.uleb128 0x23
	.long	.LBB248
	.long	.LBE248
	.uleb128 0x25
	.long	0x222
	.byte	0x2
	.byte	0x8c
	.sleb128 7
	.uleb128 0x23
	.long	.LBB249
	.long	.LBE249
	.uleb128 0x27
	.long	0x22f
	.long	.LLST84
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x33
	.long	0x2fe
	.long	.Ldebug_ranges0+0xf8
	.byte	0x1
	.word	0x2a0
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF65
	.byte	0x1
	.word	0x2a5
	.byte	0x1
	.long	.LFB37
	.long	.LFE37
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xed4
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2a4
	.long	0x37
	.long	.LLST86
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x2a7
	.long	0x1ca
	.byte	0x2
	.byte	0x8c
	.sleb128 2
	.uleb128 0x23
	.long	.LBB254
	.long	.LBE254
	.uleb128 0x2a
	.string	"i"
	.byte	0x1
	.word	0x2a8
	.long	0x37
	.byte	0x2
	.byte	0x8c
	.sleb128 1
	.uleb128 0x23
	.long	.LBB255
	.long	.LBE255
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x2a9
	.long	0x37
	.uleb128 0x33
	.long	0x2fe
	.long	.Ldebug_ranges0+0x110
	.byte	0x1
	.word	0x2aa
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF66
	.byte	0x1
	.word	0x2af
	.byte	0x1
	.long	.LFB38
	.long	.LFE38
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0xf5d
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2ae
	.long	0x37
	.long	.LLST88
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x2b1
	.long	0x1ca
	.byte	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.uleb128 0x23
	.long	.LBB260
	.long	.LBE260
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x2b2
	.long	0x37
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB261
	.long	.LBE261
	.byte	0x1
	.word	0x2b3
	.long	0xf40
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x30
	.long	0x2fe
	.long	.LBB263
	.long	.LBE263
	.byte	0x1
	.word	0x2b4
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF67
	.byte	0x1
	.word	0x2b9
	.byte	0x1
	.long	.LFB39
	.long	.LFE39
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x106d
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2b8
	.long	0x37
	.long	.LLST90
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x2bb
	.long	0x1ca
	.byte	0x6
	.byte	0x68
	.byte	0x93
	.uleb128 0x1
	.byte	0x69
	.byte	0x93
	.uleb128 0x1
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB265
	.long	.LBE265
	.byte	0x1
	.word	0x2bc
	.long	0xfb6
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB267
	.long	.LBE267
	.byte	0x1
	.word	0x2bd
	.long	0xfd5
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB269
	.long	.LBE269
	.byte	0x1
	.word	0x2be
	.long	0xff4
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB271
	.long	.LBE271
	.byte	0x1
	.word	0x2bf
	.long	0x1013
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB273
	.long	.LBE273
	.byte	0x1
	.word	0x2c0
	.long	0x1032
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x2c
	.long	0x2fe
	.long	.LBB275
	.long	.LBE275
	.byte	0x1
	.word	0x2c1
	.long	0x1051
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.uleb128 0x30
	.long	0x2fe
	.long	.LBB277
	.long	.LBE277
	.byte	0x1
	.word	0x2c2
	.uleb128 0x1d
	.long	0x318
	.uleb128 0x1d
	.long	0x30c
	.byte	0x0
	.byte	0x0
	.uleb128 0x34
	.byte	0x1
	.long	.LASF136
	.byte	0x1
	.word	0x2c6
	.long	.LFB40
	.long	.LFE40
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.uleb128 0x28
	.byte	0x1
	.long	.LASF68
	.byte	0x1
	.word	0x2cd
	.byte	0x1
	.long	.LFB41
	.long	.LFE41
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x10ab
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2cc
	.long	0x37
	.long	.LLST93
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF69
	.byte	0x1
	.word	0x2e3
	.byte	0x1
	.long	.LFB42
	.long	.LFE42
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x117b
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2e2
	.long	0x37
	.long	.LLST95
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x2e5
	.long	0x1ca
	.byte	0x2
	.byte	0x8c
	.sleb128 8
	.uleb128 0x23
	.long	.LBB279
	.long	.LBE279
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x2e7
	.long	0x37
	.uleb128 0x23
	.long	.LBB280
	.long	.LBE280
	.uleb128 0x2d
	.string	"x"
	.byte	0x1
	.word	0x2e8
	.long	0x37
	.long	.LLST96
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB281
	.long	.LBE281
	.byte	0x1
	.word	0x2e8
	.long	0x1134
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB282
	.long	.LBE282
	.uleb128 0x26
	.long	0x1ec
	.byte	0x0
	.byte	0x0
	.uleb128 0x23
	.long	.LBB283
	.long	.LBE283
	.uleb128 0x15
	.string	"y"
	.byte	0x1
	.word	0x2e9
	.long	0x37
	.uleb128 0x23
	.long	.LBB284
	.long	.LBE284
	.uleb128 0x2d
	.string	"t"
	.byte	0x1
	.word	0x2ea
	.long	0x8f
	.long	.LLST97
	.uleb128 0x23
	.long	.LBB285
	.long	.LBE285
	.uleb128 0x2a
	.string	"val"
	.byte	0x1
	.word	0x2ed
	.long	0x37
	.byte	0x1
	.byte	0x5a
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF70
	.byte	0x1
	.word	0x2f3
	.byte	0x1
	.long	.LFB43
	.long	.LFE43
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x1268
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2f2
	.long	0x37
	.long	.LLST99
	.uleb128 0x15
	.string	"p"
	.byte	0x1
	.word	0x2f5
	.long	0x1ca
	.uleb128 0x23
	.long	.LBB286
	.long	.LBE286
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x2f6
	.long	0x37
	.uleb128 0x23
	.long	.LBB287
	.long	.LBE287
	.uleb128 0x2d
	.string	"y"
	.byte	0x1
	.word	0x2f7
	.long	0x37
	.long	.LLST100
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB288
	.long	.LBE288
	.byte	0x1
	.word	0x2f7
	.long	0x1201
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB289
	.long	.LBE289
	.uleb128 0x26
	.long	0x1ec
	.byte	0x0
	.byte	0x0
	.uleb128 0x23
	.long	.LBB290
	.long	.LBE290
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x2f8
	.long	0x37
	.uleb128 0x1e
	.long	.LBB291
	.long	.LBE291
	.long	0x123e
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x2f9
	.long	0x5b
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x2f9
	.long	0x37
	.long	.LLST101
	.byte	0x0
	.uleb128 0x23
	.long	.LBB292
	.long	.LBE292
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x2fa
	.long	0x5b
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x2fa
	.long	0x37
	.long	.LLST102
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF71
	.byte	0x1
	.word	0x300
	.byte	0x1
	.long	.LFB44
	.long	.LFE44
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x1348
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x2ff
	.long	0x37
	.long	.LLST104
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x302
	.long	0x1ca
	.byte	0x2
	.byte	0x8c
	.sleb128 3
	.uleb128 0x23
	.long	.LBB293
	.long	.LBE293
	.uleb128 0x2d
	.string	"i"
	.byte	0x1
	.word	0x303
	.long	0x37
	.long	.LLST105
	.uleb128 0x23
	.long	.LBB294
	.long	.LBE294
	.uleb128 0x2d
	.string	"x"
	.byte	0x1
	.word	0x304
	.long	0x37
	.long	.LLST106
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB295
	.long	.LBE295
	.byte	0x1
	.word	0x304
	.long	0x12f5
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB296
	.long	.LBE296
	.uleb128 0x26
	.long	0x1ec
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB297
	.long	.LBE297
	.long	0x131f
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x305
	.long	0x5b
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x305
	.long	0x37
	.long	.LLST107
	.byte	0x0
	.uleb128 0x23
	.long	.LBB298
	.long	.LBE298
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x306
	.long	0x5b
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x306
	.long	0x37
	.long	.LLST108
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF72
	.byte	0x1
	.word	0x30b
	.byte	0x1
	.long	.LFB45
	.long	.LFE45
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x13e9
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x30a
	.long	0x37
	.long	.LLST110
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x30d
	.long	0x1ca
	.byte	0x2
	.byte	0x8c
	.sleb128 6
	.uleb128 0x23
	.long	.LBB299
	.long	.LBE299
	.uleb128 0x15
	.string	"i"
	.byte	0x1
	.word	0x30e
	.long	0x37
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB300
	.long	.LBE300
	.byte	0x1
	.word	0x30f
	.long	0x13be
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB301
	.long	.LBE301
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST111
	.byte	0x0
	.byte	0x0
	.uleb128 0x30
	.long	0x1d0
	.long	.LBB302
	.long	.LBE302
	.byte	0x1
	.word	0x30f
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB303
	.long	.LBE303
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST112
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF73
	.byte	0x1
	.word	0x313
	.byte	0x1
	.long	.LFB46
	.long	.LFE46
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x1559
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x312
	.long	0x37
	.long	.LLST114
	.uleb128 0x2a
	.string	"p"
	.byte	0x1
	.word	0x315
	.long	0x1ca
	.byte	0x6
	.byte	0x6a
	.byte	0x93
	.uleb128 0x1
	.byte	0x6b
	.byte	0x93
	.uleb128 0x1
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB304
	.long	.LBE304
	.byte	0x1
	.word	0x316
	.long	0x1450
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB305
	.long	.LBE305
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST115
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB306
	.long	.LBE306
	.byte	0x1
	.word	0x316
	.long	0x147d
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB307
	.long	.LBE307
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST116
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB308
	.long	.LBE308
	.byte	0x1
	.word	0x317
	.long	0x14aa
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB309
	.long	.LBE309
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST117
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB310
	.long	.LBE310
	.byte	0x1
	.word	0x318
	.long	0x14d7
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB311
	.long	.LBE311
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST118
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB312
	.long	.LBE312
	.byte	0x1
	.word	0x319
	.long	0x1504
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB313
	.long	.LBE313
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST119
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.long	0x1d0
	.long	.LBB314
	.long	.LBE314
	.byte	0x1
	.word	0x31a
	.long	0x1531
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB315
	.long	.LBE315
	.uleb128 0x27
	.long	0x1ec
	.long	.LLST120
	.byte	0x0
	.byte	0x0
	.uleb128 0x30
	.long	0x1d0
	.long	.LBB316
	.long	.LBE316
	.byte	0x1
	.word	0x31b
	.uleb128 0x1d
	.long	0x1e1
	.uleb128 0x23
	.long	.LBB317
	.long	.LBE317
	.uleb128 0x25
	.long	0x1ec
	.byte	0x1
	.byte	0x68
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF74
	.byte	0x1
	.word	0x31f
	.byte	0x1
	.long	.LFB47
	.long	.LFE47
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x1583
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x31e
	.long	0x37
	.long	.LLST122
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF75
	.byte	0x1
	.word	0x236
	.byte	0x1
	.long	.LFB34
	.long	.LFE34
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x1633
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x235
	.long	0x37
	.long	.LLST124
	.uleb128 0x2d
	.string	"i"
	.byte	0x1
	.word	0x237
	.long	0x54
	.long	.LLST125
	.uleb128 0x1e
	.long	.LBB318
	.long	.LBE318
	.long	0x15d8
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x277
	.long	0x12b
	.long	.LLST126
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB319
	.long	.LBE319
	.long	0x15f6
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x27e
	.long	0x12b
	.long	.LLST127
	.byte	0x0
	.uleb128 0x30
	.long	0x325
	.long	.LBB320
	.long	.LBE320
	.byte	0x1
	.word	0x23b
	.uleb128 0x23
	.long	.LBB322
	.long	.LBE322
	.uleb128 0x27
	.long	0x334
	.long	.LLST128
	.uleb128 0x23
	.long	.LBB323
	.long	.LBE323
	.uleb128 0x26
	.long	0x33f
	.uleb128 0x27
	.long	0x34b
	.long	.LLST129
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.byte	0x1
	.long	.LASF76
	.byte	0x1
	.word	0x32c
	.byte	0x1
	.long	.LFB48
	.long	.LFE48
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x165d
	.uleb128 0x29
	.long	.LASF37
	.byte	0x1
	.word	0x32b
	.long	0x37
	.long	.LLST131
	.byte	0x0
	.uleb128 0x2b
	.byte	0x1
	.long	.LASF77
	.byte	0x1
	.word	0x336
	.long	.LFB49
	.long	.LFE49
	.byte	0x2
	.byte	0x90
	.uleb128 0x20
	.long	0x17ce
	.uleb128 0x1e
	.long	.LBB324
	.long	.LBE324
	.long	0x1693
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x349
	.long	0x37
	.long	.LLST133
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB325
	.long	.LBE325
	.long	0x16ab
	.uleb128 0x15
	.string	"x"
	.byte	0x1
	.word	0x34e
	.long	0x37
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB326
	.long	.LBE326
	.long	0x16c9
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x353
	.long	0x12b
	.long	.LLST134
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB327
	.long	.LBE327
	.long	0x170d
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x361
	.long	0x5b
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x361
	.long	0x37
	.long	.LLST135
	.uleb128 0x23
	.long	.LBB328
	.long	.LBE328
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x361
	.long	0x37
	.long	.LLST136
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB329
	.long	.LBE329
	.long	0x1751
	.uleb128 0xd
	.long	.LASF27
	.byte	0x1
	.word	0x362
	.long	0x5b
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x362
	.long	0x37
	.long	.LLST137
	.uleb128 0x23
	.long	.LBB330
	.long	.LBE330
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x362
	.long	0x37
	.long	.LLST138
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.long	.LBB331
	.long	.LBE331
	.long	0x1799
	.uleb128 0x2f
	.long	.LASF27
	.byte	0x1
	.word	0x363
	.long	0x5b
	.long	.LLST139
	.uleb128 0x2f
	.long	.LASF28
	.byte	0x1
	.word	0x363
	.long	0x5b
	.long	.LLST140
	.uleb128 0x23
	.long	.LBB332
	.long	.LBE332
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x363
	.long	0x37
	.long	.LLST141
	.byte	0x0
	.byte	0x0
	.uleb128 0x23
	.long	.LBB333
	.long	.LBE333
	.uleb128 0x2f
	.long	.LASF37
	.byte	0x1
	.word	0x355
	.long	0x37
	.long	.LLST142
	.uleb128 0x23
	.long	.LBB334
	.long	.LBE334
	.uleb128 0x2d
	.string	"val"
	.byte	0x1
	.word	0x355
	.long	0x37
	.long	.LLST143
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.uleb128 0x35
	.long	.LASF78
	.byte	0x1
	.byte	0x86
	.long	0x1fe
	.byte	0x1
	.uleb128 0x35
	.long	.LASF79
	.byte	0x1
	.byte	0x87
	.long	0x1fe
	.byte	0xf
	.uleb128 0x36
	.long	0x37
	.long	0x17f6
	.uleb128 0x37
	.long	0x17f6
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.byte	0x7
	.uleb128 0x38
	.long	.LASF80
	.byte	0x1
	.byte	0x17
	.long	0x17e6
	.byte	0x1
	.byte	0x1
	.uleb128 0x36
	.long	0x25
	.long	0x1816
	.uleb128 0x37
	.long	0x17f6
	.byte	0xff
	.byte	0x0
	.uleb128 0x38
	.long	.LASF81
	.byte	0x1
	.byte	0x18
	.long	0x1806
	.byte	0x1
	.byte	0x1
	.uleb128 0x36
	.long	0x37
	.long	0x1839
	.uleb128 0x37
	.long	0x17f6
	.byte	0x1
	.uleb128 0x37
	.long	0x17f6
	.byte	0xff
	.byte	0x0
	.uleb128 0x38
	.long	.LASF82
	.byte	0x1
	.byte	0x19
	.long	0x1846
	.byte	0x1
	.byte	0x1
	.uleb128 0x39
	.long	0x1823
	.uleb128 0x36
	.long	0x25
	.long	0x1861
	.uleb128 0x37
	.long	0x17f6
	.byte	0x1
	.uleb128 0x37
	.long	0x17f6
	.byte	0xff
	.byte	0x0
	.uleb128 0x38
	.long	.LASF83
	.byte	0x1
	.byte	0x1a
	.long	0x186e
	.byte	0x1
	.byte	0x1
	.uleb128 0x39
	.long	0x184b
	.uleb128 0x36
	.long	0x37
	.long	0x1883
	.uleb128 0x37
	.long	0x17f6
	.byte	0xff
	.byte	0x0
	.uleb128 0x38
	.long	.LASF84
	.byte	0x1
	.byte	0x1b
	.long	0x1890
	.byte	0x1
	.byte	0x1
	.uleb128 0x39
	.long	0x1873
	.uleb128 0x38
	.long	.LASF85
	.byte	0x1
	.byte	0x1c
	.long	0x1873
	.byte	0x1
	.byte	0x1
	.uleb128 0x36
	.long	0x5b
	.long	0x18b2
	.uleb128 0x37
	.long	0x17f6
	.byte	0xf
	.byte	0x0
	.uleb128 0x38
	.long	.LASF86
	.byte	0x1
	.byte	0x1d
	.long	0x18a2
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF87
	.byte	0x1
	.byte	0x1e
	.long	0x37
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF88
	.byte	0x1
	.byte	0x1f
	.long	0x37
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF89
	.byte	0x1
	.byte	0x20
	.long	0x18e6
	.byte	0x1
	.byte	0x1
	.uleb128 0x39
	.long	0x37
	.uleb128 0x38
	.long	.LASF90
	.byte	0x1
	.byte	0x21
	.long	0x18e6
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF91
	.byte	0x1
	.byte	0x22
	.long	0x1905
	.byte	0x1
	.byte	0x1
	.uleb128 0x39
	.long	0x5b
	.uleb128 0x38
	.long	.LASF92
	.byte	0x1
	.byte	0x23
	.long	0x18e6
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF93
	.byte	0x1
	.byte	0x24
	.long	0x37
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF94
	.byte	0x1
	.byte	0x25
	.long	0x5b
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF95
	.byte	0x1
	.byte	0x27
	.long	0x37
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF96
	.byte	0x1
	.byte	0x28
	.long	0x12b
	.byte	0x1
	.byte	0x1
	.uleb128 0x3a
	.long	.LASF97
	.byte	0x1
	.byte	0x49
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x5e
	.uleb128 0x38
	.long	.LASF98
	.byte	0x1
	.byte	0x2a
	.long	0x37
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF99
	.byte	0x1
	.byte	0x2b
	.long	0x37
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF100
	.byte	0x1
	.byte	0x2c
	.long	0x17e6
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF101
	.byte	0x1
	.byte	0x2d
	.long	0x18e6
	.byte	0x1
	.byte	0x1
	.uleb128 0x38
	.long	.LASF102
	.byte	0x1
	.byte	0x2f
	.long	0x199a
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x1806
	.uleb128 0x36
	.long	0x37
	.long	0x19af
	.uleb128 0x37
	.long	0x17f6
	.byte	0x1f
	.byte	0x0
	.uleb128 0x38
	.long	.LASF103
	.byte	0x1
	.byte	0x32
	.long	0x19bc
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x199f
	.uleb128 0x38
	.long	.LASF104
	.byte	0x1
	.byte	0x33
	.long	0x19ce
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x199f
	.uleb128 0x38
	.long	.LASF105
	.byte	0x1
	.byte	0x34
	.long	0x19e0
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x199f
	.uleb128 0x36
	.long	0x25
	.long	0x19f5
	.uleb128 0x37
	.long	0x17f6
	.byte	0xf
	.byte	0x0
	.uleb128 0x38
	.long	.LASF106
	.byte	0x1
	.byte	0x35
	.long	0x1a02
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x19e5
	.uleb128 0x38
	.long	.LASF107
	.byte	0x1
	.byte	0x36
	.long	0x1a14
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x17e6
	.uleb128 0x38
	.long	.LASF108
	.byte	0x1
	.byte	0x37
	.long	0x1a26
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x17e6
	.uleb128 0x38
	.long	.LASF109
	.byte	0x1
	.byte	0x38
	.long	0x1a38
	.byte	0x1
	.byte	0x1
	.uleb128 0x13
	.long	0x18a2
	.uleb128 0x3a
	.long	.LASF110
	.byte	0x1
	.byte	0x3d
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x52
	.uleb128 0x3a
	.long	.LASF111
	.byte	0x1
	.byte	0x3e
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x53
	.uleb128 0x3a
	.long	.LASF112
	.byte	0x1
	.byte	0x3f
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x54
	.uleb128 0x3a
	.long	.LASF113
	.byte	0x1
	.byte	0x40
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x55
	.uleb128 0x3a
	.long	.LASF114
	.byte	0x1
	.byte	0x41
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x56
	.uleb128 0x3a
	.long	.LASF115
	.byte	0x1
	.byte	0x42
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x57
	.uleb128 0x3a
	.long	.LASF116
	.byte	0x1
	.byte	0x43
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x58
	.uleb128 0x3a
	.long	.LASF117
	.byte	0x1
	.byte	0x44
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x59
	.uleb128 0x3a
	.long	.LASF118
	.byte	0x1
	.byte	0x45
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x5a
	.uleb128 0x3a
	.long	.LASF119
	.byte	0x1
	.byte	0x46
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x5b
	.uleb128 0x3a
	.long	.LASF37
	.byte	0x1
	.byte	0x47
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x5c
	.uleb128 0x3a
	.long	.LASF120
	.byte	0x1
	.byte	0x48
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x5d
	.uleb128 0x3a
	.long	.LASF121
	.byte	0x1
	.byte	0x4a
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x5f
	.uleb128 0x3a
	.long	.LASF122
	.byte	0x1
	.byte	0x4b
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x60
	.uleb128 0x3a
	.long	.LASF123
	.byte	0x1
	.byte	0x4c
	.long	0x12b
	.byte	0x1
	.byte	0x1
	.byte	0x61
	.uleb128 0x3a
	.long	.LASF124
	.byte	0x1
	.byte	0x4d
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x62
	.uleb128 0x3a
	.long	.LASF125
	.byte	0x1
	.byte	0x4e
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x63
	.uleb128 0x3a
	.long	.LASF126
	.byte	0x1
	.byte	0x52
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x64
	.uleb128 0x3a
	.long	.LASF127
	.byte	0x1
	.byte	0x53
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x65
	.uleb128 0x3a
	.long	.LASF128
	.byte	0x1
	.byte	0x54
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x66
	.uleb128 0x3a
	.long	.LASF129
	.byte	0x1
	.byte	0x55
	.long	0x37
	.byte	0x1
	.byte	0x1
	.byte	0x67
	.byte	0x0
	.section	.debug_abbrev
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2
	.uleb128 0x16
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0x0
	.byte	0x0
	.uleb128 0x4
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0x0
	.byte	0x0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x7
	.uleb128 0xd
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0xd
	.uleb128 0xb
	.uleb128 0xc
	.uleb128 0xb
	.uleb128 0x38
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x8
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x9
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xa
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xb
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xc
	.uleb128 0xb
	.byte	0x1
	.byte	0x0
	.byte	0x0
	.uleb128 0xd
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xe
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0xf
	.uleb128 0xf
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x10
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x11
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x12
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x13
	.uleb128 0x26
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x14
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x15
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x16
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x17
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x18
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x19
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1a
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x1b
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1c
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1d
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1e
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x1f
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x20
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x21
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x22
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x23
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.byte	0x0
	.byte	0x0
	.uleb128 0x24
	.uleb128 0x2e
	.byte	0x0
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x25
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x26
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x27
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x28
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x29
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2a
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x2b
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x2c
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x2d
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2e
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2f
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x30
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.byte	0x0
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x32
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x33
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.byte	0x0
	.byte	0x0
	.uleb128 0x34
	.uleb128 0x2e
	.byte	0x0
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x35
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x36
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x37
	.uleb128 0x21
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x38
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3c
	.uleb128 0xc
	.byte	0x0
	.byte	0x0
	.uleb128 0x39
	.uleb128 0x35
	.byte	0x0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x3a
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x433
	.word	0x2
	.long	.Ldebug_info0
	.long	0x1b64
	.long	0x35a
	.string	"encodeHex"
	.long	0x378
	.string	"sendNextByte"
	.long	0x43f
	.string	"decodeHex"
	.long	0x45d
	.string	"__vector_18"
	.long	0x4df
	.string	"__vector_19"
	.long	0x4f4
	.string	"copySine"
	.long	0x537
	.string	"drawMenu"
	.long	0x5a0
	.string	"highlight"
	.long	0x616
	.string	"coarseWaveformValue"
	.long	0x64e
	.string	"drawEscapeMenu"
	.long	0x704
	.string	"drawEEPROMScreen"
	.long	0x7a7
	.string	"startPatternEditor"
	.long	0x822
	.string	"startCoarseEditor"
	.long	0x887
	.string	"startFineEditor"
	.long	0x8af
	.string	"startTuningEditor"
	.long	0x8ce
	.string	"startMiscellaneousEditor"
	.long	0x952
	.string	"startEditor"
	.long	0x9b8
	.string	"microtoneModeTouch"
	.long	0x9f2
	.string	"patternEditorTouch"
	.long	0xaf7
	.string	"coarseEditorTouch"
	.long	0xb93
	.string	"fineEditorTouch"
	.long	0xbce
	.string	"tuningEditorTouch"
	.long	0xbf8
	.string	"miscellaneousEditorTouch"
	.long	0xc4b
	.string	"editorTouch"
	.long	0xcb6
	.string	"savePattern"
	.long	0xd63
	.string	"saveCoarse"
	.long	0xe5b
	.string	"saveFine"
	.long	0xed4
	.string	"saveTuning"
	.long	0xf5d
	.string	"saveMiscellaneous"
	.long	0x106d
	.string	"startEscapeMode"
	.long	0x1081
	.string	"save"
	.long	0x10ab
	.string	"loadPattern"
	.long	0x117b
	.string	"loadCoarse"
	.long	0x1268
	.string	"loadFine"
	.long	0x1348
	.string	"loadTuning"
	.long	0x13e9
	.string	"loadMiscellaneous"
	.long	0x1559
	.string	"load"
	.long	0x1583
	.string	"escapeModeTouch"
	.long	0x1633
	.string	"touch"
	.long	0x165d
	.string	"idleLoop"
	.long	0x194b
	.string	"beatsPerPattern"
	.long	0x1a3d
	.string	"const0"
	.long	0x1a4b
	.string	"sampleInLine"
	.long	0x1a59
	.string	"lineInFrame"
	.long	0x1a67
	.string	"frameInBeatLow"
	.long	0x1a75
	.string	"frameInBeatHigh"
	.long	0x1a83
	.string	"framesPerBeatLow"
	.long	0x1a91
	.string	"framesPerBeatHigh"
	.long	0x1a9f
	.string	"beatInPattern"
	.long	0x1aad
	.string	"lightPage"
	.long	0x1abb
	.string	"switchInFrame"
	.long	0x1ac9
	.string	"lastSwitch"
	.long	0x1ad7
	.string	"switchesTouched"
	.long	0x1ae5
	.string	"waterPage"
	.long	0x1af3
	.string	"lightsLit"
	.long	0x1b01
	.string	"flags"
	.long	0x1b0f
	.string	"const1"
	.long	0x1b1d
	.string	"const0x10"
	.long	0x1b2b
	.string	"scratch20"
	.long	0x1b39
	.string	"scratch21"
	.long	0x1b47
	.string	"scratch22"
	.long	0x1b55
	.string	"scratch23"
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x1c
	.word	0x2
	.long	.Ldebug_info0
	.byte	0x4
	.byte	0x0
	.word	0x0
	.word	0x0
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	0x0
	.long	0x0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.long	.LBB145-.Ltext0
	.long	.LBE145-.Ltext0
	.long	.LBB159-.Ltext0
	.long	.LBE159-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB148-.Ltext0
	.long	.LBE148-.Ltext0
	.long	.LBB160-.Ltext0
	.long	.LBE160-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB155-.Ltext0
	.long	.LBE155-.Ltext0
	.long	.LBB158-.Ltext0
	.long	.LBE158-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB161-.Ltext0
	.long	.LBE161-.Ltext0
	.long	.LBB166-.Ltext0
	.long	.LBE166-.Ltext0
	.long	.LBB165-.Ltext0
	.long	.LBE165-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB187-.Ltext0
	.long	.LBE187-.Ltext0
	.long	.LBB194-.Ltext0
	.long	.LBE194-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB188-.Ltext0
	.long	.LBE188-.Ltext0
	.long	.LBB193-.Ltext0
	.long	.LBE193-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB216-.Ltext0
	.long	.LBE216-.Ltext0
	.long	.LBB217-.Ltext0
	.long	.LBE217-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB218-.Ltext0
	.long	.LBE218-.Ltext0
	.long	.LBB223-.Ltext0
	.long	.LBE223-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB219-.Ltext0
	.long	.LBE219-.Ltext0
	.long	.LBB222-.Ltext0
	.long	.LBE222-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB220-.Ltext0
	.long	.LBE220-.Ltext0
	.long	.LBB221-.Ltext0
	.long	.LBE221-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB250-.Ltext0
	.long	.LBE250-.Ltext0
	.long	.LBB253-.Ltext0
	.long	.LBE253-.Ltext0
	.long	0x0
	.long	0x0
	.long	.LBB256-.Ltext0
	.long	.LBE256-.Ltext0
	.long	.LBB259-.Ltext0
	.long	.LBE259-.Ltext0
	.long	0x0
	.long	0x0
	.section	.debug_line
	.long	.LELT0-.LSLT0
.LSLT0:
	.word	0x2
	.long	.LELTP0-.LASLTP0
.LASLTP0:
	.byte	0x1
	.byte	0x1
	.byte	0xf6
	.byte	0xf5
	.byte	0xa
	.byte	0x0
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x1
	.ascii	"c:/program files/arduino/hardware/tools/avr/lib/gcc/../../av"
	.ascii	"r/include"
	.byte	0
	.ascii	"c:/program files/arduino/hardware/tools/avr/lib/gcc/../../av"
	.ascii	"r/include/avr"
	.byte	0
	.byte	0x0
	.string	"v2c.c"
	.uleb128 0x0
	.uleb128 0x0
	.uleb128 0x0
	.string	"avr/eeprom.h"
	.uleb128 0x1
	.uleb128 0x0
	.uleb128 0x0
	.string	"stdint.h"
	.uleb128 0x1
	.uleb128 0x0
	.uleb128 0x0
	.byte	0x0
.LELTP0:
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM1
	.byte	0x9d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM2
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM3
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM4
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM5
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM6
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM7
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM8
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM9
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM10
	.byte	0x3
	.sleb128 -15
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM11
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM12
	.byte	0x22
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM13
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM14
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM15
	.byte	0x3
	.sleb128 -11
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM16
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM17
	.byte	0x1e
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM18
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM19
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM20
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM21
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM22
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM23
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM24
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM25
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM26
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM27
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM28
	.byte	0x3
	.sleb128 -34
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM29
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM30
	.byte	0x35
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM31
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM32
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM33
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM34
	.byte	0xe
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM35
	.byte	0x3
	.sleb128 -31
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM36
	.byte	0x37
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM37
	.byte	0xc
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM38
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM39
	.byte	0xd
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM40
	.byte	0x2a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM41
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM42
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM43
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM44
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM45
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM46
	.byte	0x10
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM47
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM48
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM49
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM50
	.byte	0x3
	.sleb128 -11
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM51
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM52
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM53
	.byte	0x1e
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM54
	.byte	0x48
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM55
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM56
	.byte	0x27
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM57
	.byte	0x3
	.sleb128 -78
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM58
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM59
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM60
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM61
	.byte	0x3
	.sleb128 -13
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM62
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM63
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM64
	.byte	0x2c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM65
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM66
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM67
	.byte	0x3
	.sleb128 -26
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM68
	.byte	0x65
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM69
	.byte	0x3
	.sleb128 -81
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM70
	.byte	0x5e
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM71
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM72
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM73
	.byte	0x3
	.sleb128 -85
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM74
	.byte	0x2d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM75
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM76
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM77
	.byte	0x44
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM78
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM79
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM80
	.byte	0x3
	.sleb128 -64
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM81
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM82
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM83
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM84
	.byte	0x27
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM85
	.byte	0x1e
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM86
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM87
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM88
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM89
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM90
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM91
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM92
	.byte	0x23
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM93
	.byte	0xd
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM94
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM95
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM96
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM97
	.byte	0x3
	.sleb128 -26
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM98
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM99
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM100
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM101
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM102
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM103
	.byte	0x45
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM104
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM105
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM106
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM107
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM108
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM109
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM110
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM111
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM112
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM113
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM114
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM115
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM116
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM117
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM118
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM119
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM120
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM121
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM122
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM123
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM124
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM125
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM126
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM127
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM128
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM129
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM130
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM131
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM132
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM133
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM134
	.byte	0x3
	.sleb128 -27
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM135
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM136
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM137
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM138
	.byte	0x32
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM139
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM140
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM141
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM142
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM143
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM144
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM145
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM146
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM147
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM148
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM149
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM150
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -140
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM151
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM152
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM153
	.byte	0x4
	.uleb128 0x1
	.byte	0x9a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM154
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM155
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM156
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM157
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM158
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM159
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM160
	.byte	0x3
	.sleb128 -14
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM161
	.byte	0x27
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM162
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM163
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM164
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM165
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM166
	.byte	0x3
	.sleb128 -65
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM167
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM168
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM169
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM170
	.byte	0x5b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM171
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM172
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM173
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM174
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM175
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM176
	.byte	0x3
	.sleb128 -56
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM177
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM178
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM179
	.byte	0x4d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM180
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM181
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM182
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM183
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM184
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM185
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM186
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM187
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM188
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM189
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM190
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM191
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM192
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM193
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM194
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM195
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM196
	.byte	0x3
	.sleb128 -96
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM197
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM198
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM199
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM200
	.byte	0x79
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM201
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM202
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM203
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM204
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM205
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM206
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM207
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM208
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM209
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM210
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM211
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM212
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM213
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM214
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM215
	.byte	0xd
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM216
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM217
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM218
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM219
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM220
	.byte	0x3
	.sleb128 -32
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM221
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM222
	.byte	0x33
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM223
	.byte	0x3
	.sleb128 -24
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM224
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM225
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM226
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM227
	.byte	0x36
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM228
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM229
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM230
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM231
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM232
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM233
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM234
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM235
	.byte	0x1e
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM236
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM237
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM238
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM239
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM240
	.byte	0x3
	.sleb128 -16
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM241
	.byte	0x24
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM242
	.byte	0x3
	.sleb128 -15
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM243
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM244
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM245
	.byte	0x1e
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM246
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM247
	.byte	0x31
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM248
	.byte	0x3
	.sleb128 -19
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM249
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM250
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM251
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM252
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM253
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM254
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM255
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM256
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM257
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM258
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM259
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM260
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM261
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM262
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM263
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM264
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM265
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM266
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM267
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM268
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM269
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM270
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM271
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM272
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM273
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM274
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM275
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM276
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM277
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM278
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM279
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM280
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM281
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM282
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM283
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM284
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM285
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM286
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM287
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM288
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM289
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM290
	.byte	0x1d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM291
	.byte	0x37
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM292
	.byte	0x3
	.sleb128 -25
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM293
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM294
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM295
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM296
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM297
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM298
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM299
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM300
	.byte	0x3
	.sleb128 -39
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM301
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM302
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM303
	.byte	0x3
	.sleb128 -13
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM304
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM305
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM306
	.byte	0x1f
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM307
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM308
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM309
	.byte	0x22
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM310
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM311
	.byte	0x3
	.sleb128 -11
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM312
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM313
	.byte	0x3
	.sleb128 -22
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM314
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM315
	.byte	0x34
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM316
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM317
	.byte	0xb
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM318
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM319
	.byte	0x3
	.sleb128 -20
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM320
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM321
	.byte	0x35
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM322
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM323
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM324
	.byte	0xd
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM325
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM326
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM327
	.byte	0xe
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM328
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM329
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM330
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM331
	.byte	0x3
	.sleb128 -84
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM332
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM333
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM334
	.byte	0x66
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM335
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM336
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM337
	.byte	0x6f
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM338
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM339
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM340
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM341
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM342
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM343
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -359
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM344
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM345
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM346
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM347
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM348
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 342
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM349
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM350
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM351
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM352
	.byte	0x3
	.sleb128 -354
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM353
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM354
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM355
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM356
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM357
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM358
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -17
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM359
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM360
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM361
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 360
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM362
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -358
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM363
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM364
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 355
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM365
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM366
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM367
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM368
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM369
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -381
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM370
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM371
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM372
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 371
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM373
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -369
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM374
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM375
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 365
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM376
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM377
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM378
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM379
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM380
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -391
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM381
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM382
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM383
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM384
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM385
	.byte	0x3
	.sleb128 -15
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM386
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM387
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM388
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM389
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM390
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 375
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM391
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM392
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM393
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM394
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM395
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -400
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM396
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM397
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM398
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM399
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM400
	.byte	0x3
	.sleb128 -15
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM401
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM402
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM403
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM404
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM405
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 387
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM406
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -402
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM407
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM408
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM409
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM410
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM411
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 388
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM412
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -403
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM413
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM414
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM415
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM416
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM417
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 389
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM418
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -404
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM419
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM420
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM421
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM422
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM423
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 390
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM424
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -405
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM425
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM426
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM427
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM428
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM429
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 391
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM430
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -406
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM431
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM432
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM433
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM434
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM435
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 392
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM436
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM437
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM438
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM439
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM440
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM441
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM442
	.byte	0x1d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM443
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM444
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM445
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM446
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM447
	.byte	0xb
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM448
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM449
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM450
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM451
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM452
	.byte	0xc
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM453
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM454
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM455
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM456
	.byte	0x10
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM457
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM458
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM459
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM460
	.byte	0xe
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM461
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM462
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM463
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM464
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM465
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM466
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM467
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM468
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM469
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM470
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -544
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM471
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM472
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM473
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 538
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM474
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM475
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM476
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM477
	.byte	0x10
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM478
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM479
	.byte	0x1d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM480
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM481
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM482
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -560
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM483
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM484
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM485
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 553
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM486
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM487
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM488
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM489
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM490
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM491
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM492
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM493
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM494
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM495
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -575
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM496
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM497
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM498
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 565
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM499
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM500
	.byte	0x11
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM501
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM502
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM503
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM504
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -584
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM505
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM506
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM507
	.byte	0x3
	.sleb128 -11
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM508
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM509
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM510
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 575
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM511
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM512
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM513
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM514
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM515
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -592
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM516
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM517
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM518
	.byte	0x3
	.sleb128 -11
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM519
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM520
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM521
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 582
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM522
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -593
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM523
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM524
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM525
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 583
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM526
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -594
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM527
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM528
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM529
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 584
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM530
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -595
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM531
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM532
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM533
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 585
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM534
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -596
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM535
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM536
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM537
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 586
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM538
	.byte	0x4
	.uleb128 0x2
	.byte	0x3
	.sleb128 -597
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM539
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM540
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM541
	.byte	0x4
	.uleb128 0x1
	.byte	0x3
	.sleb128 587
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM542
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM543
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM544
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM545
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM546
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM547
	.byte	0xb
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM548
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM549
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM550
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM551
	.byte	0xc
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM552
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM553
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM554
	.byte	0x10
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM555
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM556
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM557
	.byte	0xe
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM558
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM559
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM560
	.byte	0xf
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM561
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM562
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM563
	.byte	0x3
	.sleb128 -243
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM564
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM565
	.byte	0x61
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM566
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM567
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM568
	.byte	0x3
	.sleb128 -83
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM569
	.byte	0x23
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM570
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM571
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM572
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM573
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM574
	.byte	0x3
	.sleb128 -18
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM575
	.byte	0x4b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM576
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM577
	.byte	0x3
	.sleb128 -56
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM578
	.byte	0x31
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM579
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM580
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM581
	.byte	0x1d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM582
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM583
	.byte	0x1b
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM584
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM585
	.byte	0x23
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM586
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM587
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM588
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM589
	.byte	0x3
	.sleb128 -48
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM590
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM591
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM592
	.byte	0x27
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM593
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM594
	.byte	0x23
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM595
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM596
	.byte	0xb
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM597
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM598
	.byte	0x3
	.sleb128 -45
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM599
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM600
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM601
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM602
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM603
	.byte	0x2d
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM604
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM605
	.byte	0x39
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM606
	.byte	0xd
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM607
	.byte	0x3
	.sleb128 -66
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM608
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM609
	.byte	0x3
	.sleb128 -280
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM610
	.byte	0x13
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM611
	.byte	0x3
	.sleb128 523
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM612
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM613
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM614
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM615
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM616
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM617
	.byte	0x1a
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM618
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM619
	.byte	0x18
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM620
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM621
	.byte	0x1f
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM622
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM623
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM624
	.byte	0x16
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM625
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM626
	.byte	0x12
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM627
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM628
	.byte	0x1c
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM629
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM630
	.byte	0x19
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM631
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM632
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM633
	.byte	0x3
	.sleb128 -42
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM634
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM635
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM636
	.byte	0x17
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM637
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM638
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM639
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM640
	.byte	0x2f
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM641
	.byte	0xb
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM642
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM643
	.byte	0x15
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.LM644
	.byte	0x3
	.sleb128 -18
	.byte	0x1
	.byte	0x0
	.uleb128 0x5
	.byte	0x2
	.long	.Letext0
	.byte	0x0
	.uleb128 0x1
	.byte	0x1
.LELT0:
	.section	.debug_str,"MS",@progbits,1
.LASF63:
	.string	"savePattern"
.LASF81:
	.string	"waveform"
.LASF73:
	.string	"loadMiscellaneous"
.LASF134:
	.string	"eeprom_write_byte"
.LASF41:
	.string	"fineEditorTouch"
.LASF99:
	.string	"noiseUpdatePointer"
.LASF27:
	.string	"__addr16"
.LASF33:
	.string	"startFineEditor"
.LASF21:
	.string	"encodeHex"
.LASF89:
	.string	"sendState"
.LASF12:
	.string	"bit0"
.LASF13:
	.string	"bit1"
.LASF14:
	.string	"bit2"
.LASF15:
	.string	"bit3"
.LASF16:
	.string	"bit4"
.LASF17:
	.string	"bit5"
.LASF18:
	.string	"bit6"
.LASF19:
	.string	"bit7"
.LASF87:
	.string	"editor"
.LASF85:
	.string	"frameBuffer"
.LASF50:
	.string	"drawEEPROMScreen"
.LASF10:
	.string	"long long unsigned int"
.LASF52:
	.string	"startCoarseEditor"
.LASF104:
	.string	"miscellaneousMenu"
.LASF59:
	.string	"sample"
.LASF20:
	.string	"flags_t"
.LASF4:
	.string	"int16_t"
.LASF9:
	.string	"long long int"
.LASF2:
	.string	"signed char"
.LASF116:
	.string	"framesPerBeatHigh"
.LASF108:
	.string	"greenLED"
.LASF48:
	.string	"topLeft"
.LASF69:
	.string	"loadPattern"
.LASF7:
	.string	"long int"
.LASF88:
	.string	"waveformPreset"
.LASF94:
	.string	"framesPerBeatOverride"
.LASF57:
	.string	"lightOn"
.LASF5:
	.string	"uint16_t"
.LASF84:
	.string	"switchBuffer"
.LASF34:
	.string	"startTuningEditor"
.LASF30:
	.string	"coarseWaveformValue"
.LASF115:
	.string	"framesPerBeatLow"
.LASF66:
	.string	"saveTuning"
.LASF28:
	.string	"__result"
.LASF122:
	.string	"lightsLit"
.LASF26:
	.string	"byte"
.LASF130:
	.string	"GNU C 4.3.2"
.LASF71:
	.string	"loadFine"
.LASF68:
	.string	"save"
.LASF23:
	.string	"value"
.LASF22:
	.string	"decodeHex"
.LASF6:
	.string	"unsigned int"
.LASF90:
	.string	"receiveState"
.LASF42:
	.string	"position"
.LASF112:
	.string	"lineInFrame"
.LASF56:
	.string	"command"
.LASF64:
	.string	"saveCoarse"
.LASF8:
	.string	"long unsigned int"
.LASF62:
	.string	"editorTouch"
.LASF111:
	.string	"sampleInLine"
.LASF37:
	.string	"lastSwitch"
.LASF109:
	.string	"blueLED"
.LASF127:
	.string	"scratch21"
.LASF74:
	.string	"load"
.LASF83:
	.string	"waterBuffers"
.LASF103:
	.string	"escapeMenu"
.LASF125:
	.string	"const0x10"
.LASF11:
	.string	"bool"
.LASF132:
	.string	"Q:\\\\reenigne\\\\tone_matrix"
.LASF60:
	.string	"tuningEditorTouch"
.LASF32:
	.string	"total"
.LASF96:
	.string	"flags2"
.LASF102:
	.string	"sineTable"
.LASF58:
	.string	"coarseEditorTouch"
.LASF36:
	.string	"microtoneModeTouch"
.LASF121:
	.string	"waterPage"
.LASF67:
	.string	"saveMiscellaneous"
.LASF95:
	.string	"decayConstantOverride"
.LASF92:
	.string	"debugValue"
.LASF76:
	.string	"touch"
.LASF91:
	.string	"debugAddress"
.LASF45:
	.string	"sendNextByte"
.LASF44:
	.string	"copySine"
.LASF55:
	.string	"patternEditorTouch"
.LASF72:
	.string	"loadTuning"
.LASF101:
	.string	"serialLED"
.LASF105:
	.string	"microtoneScreen"
.LASF133:
	.string	"eeprom_read_byte"
.LASF100:
	.string	"microtoneKeys"
.LASF119:
	.string	"switchInFrame"
.LASF70:
	.string	"loadCoarse"
.LASF82:
	.string	"lightBuffers"
.LASF80:
	.string	"volumes"
.LASF98:
	.string	"patternsPerLoop"
.LASF39:
	.string	"minVolumeChannel"
.LASF126:
	.string	"scratch20"
.LASF120:
	.string	"switchesTouched"
.LASF128:
	.string	"scratch22"
.LASF129:
	.string	"scratch23"
.LASF3:
	.string	"unsigned char"
.LASF53:
	.string	"startMiscellaneousEditor"
.LASF47:
	.string	"highlight"
.LASF49:
	.string	"drawEscapeMenu"
.LASF40:
	.string	"channel"
.LASF43:
	.string	"__value"
.LASF78:
	.string	"normalBrightness"
.LASF117:
	.string	"beatInPattern"
.LASF131:
	.string	"v2c.c"
.LASF110:
	.string	"const0"
.LASF124:
	.string	"const1"
.LASF93:
	.string	"mode"
.LASF75:
	.string	"escapeModeTouch"
.LASF106:
	.string	"speakerPositions"
.LASF107:
	.string	"redLED"
.LASF136:
	.string	"startEscapeMode"
.LASF29:
	.string	"column"
.LASF24:
	.string	"received"
.LASF31:
	.string	"part"
.LASF0:
	.string	"int8_t"
.LASF79:
	.string	"fullBrightness"
.LASF54:
	.string	"startEditor"
.LASF97:
	.string	"beatsPerPattern"
.LASF118:
	.string	"lightPage"
.LASF51:
	.string	"startPatternEditor"
.LASF35:
	.string	"drawMenu"
.LASF1:
	.string	"uint8_t"
.LASF123:
	.string	"flags"
.LASF25:
	.string	"menu"
.LASF114:
	.string	"frameInBeatHigh"
.LASF46:
	.string	"__vector_18"
.LASF135:
	.string	"__vector_19"
.LASF61:
	.string	"miscellaneousEditorTouch"
.LASF77:
	.string	"idleLoop"
.LASF113:
	.string	"frameInBeatLow"
.LASF65:
	.string	"saveFine"
.LASF38:
	.string	"minVolume"
.LASF86:
	.string	"frequencies"
