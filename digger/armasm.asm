; Digger Remastered
; Copyright (c) Andrew Jenner 1998-2004
;
;     Name: armasm.s
;
;     Date: Tue Oct  6 20:35:59 1998
;

OS_WriteC		EQU 0x0
OS_CLI			EQU 0x05
OS_Byte                 EQU 0x06
OS_Word                 EQU 0x07
OS_ReadVduVariables     EQU 0x31
OS_CheckModeValid       EQU 0x3f
OS_ReadMonotonicTime    EQU 0x42
OS_ScreenMode           EQU 0x65
Wimp_SetPointerShape    EQU 0x400d8

PCSpeakerEmu_Initialise EQU 0x4e0c0
PCSpeakerEmu_CloseDown  EQU 0x4e0c1
PCSpeakerEmu_Sound      EQU 0x4e0c2


  AREA |data|, DATA

lintologvolumetab
  DCD 0,22,35,45,52,58,63,67,71,74,77,80,83,85,87,90,91,93,95,97,98,100,101
  DCD 103,104,105,106,108,109,110,111,112,113,114,115,116,117,118,118,119,120
  DCD 121,122,122,123,124,124,125,126,127

modeblock_640x400
	DCD 1
	DCD 640
	DCD 400
	DCD 2
	DCD -1
	DCD -1

modeblock_640x480
	DCD 1
	DCD 640
	DCD 480
	DCD 2
	DCD -1
	DCD -1

wipesize
	DCD 128000

vgacolours
  DCB  0, 0, 0,  0, 0,32,  0,32, 0,  0,32,32
  DCB 32, 0, 0, 32, 0,32, 32,24, 0, 32,32,32
  DCB 16,16,16,  0, 0,63,  0,63, 0,  0,63,63
  DCB 63, 0, 0, 63, 0,63, 63,63, 0, 63,63,63
  DCB  0, 0, 0,  0, 0,63,  0,63, 0,  0,63,63
  DCB 63, 0, 0, 63, 0,63, 63,63, 0, 48,48,48
  DCB 32,32,32, 32,32,63, 32,63,32, 32,63,63
  DCB 63,32,32, 63,32,63, 63,63,32, 63,63,63
  DCB  0, 0, 0,  0,32, 0, 32, 0, 0, 32,16, 0
  DCB  0, 0,32,  0,32,32, 32, 0,32, 32,32,32
  DCB 16,16,16,  0,63, 0, 63, 0, 0, 63,63, 0
  DCB  0, 0,63,  0,63,63, 63, 0,63, 63,63,63
  DCB  0, 0, 0,  0,63, 0, 63, 0, 0, 63,32, 0
  DCB  0, 0,63,  0,63,63, 63, 0,63, 48,48,48
  DCB 32,32,32, 32,63,32, 63,32,32, 63,63,32
  DCB 32,32,63, 32,63,63, 63,32,63, 63,63,63

palettestatus
	DCD 0

palbuffer
	% 20

modeinfo
	DCD 149
	DCD -1

screenbase
	DCD 0

key7string
	DCB "key7 ",137,0
	ALIGN

key8string
	DCB "key8 ",138,0
	ALIGN

key9string
	DCB "key9 ",139,0
	ALIGN

key10string
	DCB "key10 ",140,0
	ALIGN


	AREA |code|, CODE, READONLY

  EXPORT olddelay
  EXPORT getkips
  EXPORT inittimer
  EXPORT gethrt
  EXPORT getlrt
  EXPORT s0initint8
  EXPORT s0restoreint8
  EXPORT s0setspkrt2
  EXPORT s0settimer0
  EXPORT s0timer0
  EXPORT s0settimer2
  EXPORT s0timer2
  EXPORT s0soundinitglob
  EXPORT s0soundkillglob
  EXPORT s0soundoff
  EXPORT initkeyb
  EXPORT restorekeyb
  EXPORT inkey
  EXPORT graphicsoff
  EXPORT gretrace
  EXPORT vgainit
	EXPORT vgaclear
	EXPORT vgapal
	EXPORT vgainten
	EXPORT vgaputi
	EXPORT vgageti
	EXPORT vgaputim2
	EXPORT vgagetpix
	EXPORT vgatitle
	EXPORT vgawrite
  EXPORT cgainit
  EXPORT cgaclear
  EXPORT cgapal
  EXPORT cgainten
  EXPORT cgaputi
  EXPORT cgageti
  EXPORT cgaputim2
  EXPORT cgagetpix
  EXPORT cgatitle
  EXPORT cgawrite

  IMPORT pulsewidth
  IMPORT convertfunction
  IMPORT vgatable
  IMPORT vgatitledat
  IMPORT ascii2vga
  IMPORT textoffdat

olddelay
  movs pc,r14

getkips
  mov r0,#0
  movs pc,r14

inittimer
  movs pc,r14

gethrt
getlrt
	swi OS_ReadMonotonicTime
	movs pc,r14


s0soundinitglob
  swi PCSpeakerEmu_Initialise
  movs pc,r14


s0soundkillglob
  swi PCSpeakerEmu_CloseDown
  movs pc,r14


s0soundoff
  stmfd r13!,{r14}
  bl s0soundoff0
  bl s0soundoff2
  ldmfd r13!,{pc}


s0soundoff0
  mov r0,#2
  mov r1,#0
  mov r2,#0
  mov r3,#0
  swi PCSpeakerEmu_Sound
  movs pc,r14


s0soundoff2
  mov r0,#3
  mov r1,#0
  mov r2,#0
  mov r3,#0
  swi PCSpeakerEmu_Sound
  movs pc,r14


s0settimer0
s0timer0
  stmfd r13!,{r4,r14}
  mov r4,r0
  cmp r0,#40
  blle s0soundoff0
  ldmlefd r13!,{r4,pc}
  mov r0,r4
  bl convertfunction
  mov r2,r0
  adrl r1,pulsewidth
  ldr r1,[r1]
  adrl r4,lintologvolumetab
  sub r1,r1,#1
  ldr r1,[r4,r1,lsl #2]

  add r1,r1,#&100
  mov r3,#255
  mov r0,#2
  swi PCSpeakerEmu_Sound
  ldmfd r13!,{r4,pc}


s0settimer2
s0timer2
  stmfd r13!,{r4,r14}
  mov r4,r0
  cmp r0,#40
  blle s0soundoff2
  ldmlefd r13!,{r4,pc}
  mov r0,r4
  bl convertfunction
  mov r2,r0
  mov r1,#100
  add r1,r1,#&100

  mov r3,#255
  mov r0,#3
  swi PCSpeakerEmu_Sound
  ldmfd r13!,{r4,pc}


s0initint8
s0restoreint8
s0setspkrt2
  movs pc,r14


initkeyb
  adrl r0,key7string
  swi OS_CLI
  adrl r0,key8string
  swi OS_CLI
  adrl r0,key9string
  swi OS_CLI
  adrl r0,key10string
  swi OS_CLI
  movs pc,r14

restorekeyb
  movs pc,r14


inkey
  mov r0,#129
  mov r1,#0
  mov r2,#0
  swi OS_Byte
  and r0,r1,#255
  movs pc,r14


gretrace
	mov r0,#19
	swi OS_Byte
	movs pc,r14


graphicsoff
	mov r0,#4
	swi OS_WriteC
  movs pc,r14


cgainit
vgainit
	stmfd r13!,{r4,r5,r14}

	; check for pre-RiscPC here...

	mov r4,#0
	mov r0,#27	; old computer?
  swi OS_CheckModeValid
	cmp r0,#0
	orrgt r4,r4,#1	; bit 0 if mode 27 ok

	adrl r0,modeblock_640x480
  swi OS_CheckModeValid
	cmp r0,#0
	orrgt r4,r4,#2	; bit 1 if 640x480 ok

	adrl r0,modeblock_640x400
  swi OS_CheckModeValid
	cmp r0,#0
	orrgt r4,r4,#4	; bit 2 if 640x400 ok

	cmp r4,#1	; probably a pre-Risc PC machine
	swieq 256+22
	swieq 256+27	; set mode 27
	beq modehasbeenset

	; use first match (arghh)
	tst r4,#2
	adrnel r1,modeblock_640x480
	tst r4,#4
	adrnel r1,modeblock_640x400

	mov r0,#0	; reason code 0 - select mode
  swi OS_ScreenMode

	adrl r0,modeinfo
	mov r1,r0
	swi OS_ReadVduVariables
	ldr r0,[r0]
	adrl r1,screenbase
	str r0,[r1]

modehasbeenset
; now need to (1) disable pointer...
	mov r0,#0
	mvn r1,#0
	mov r2,#16
	mov r3,#16
	mov r4,#0
	mov r5,#0
  swi Wimp_SetPointerShape  ;(deprecated system call!)
; and (2) turn off the cursor
	swi 256+23	; misc VDU
	swi 256+1	; Cursor appearance
	swi 256+0	; Hide cursor
	swi 256+0	; 9 padding zeros
	swi 256+0
	swi 256+0
	swi 256+0
	swi 256+0
	swi 256+0
	swi 256+0
	swi 256+5	; cursor-graphics mode (cancels cursor editing)
	ldmfd r13!,{r4,r5,pc}


cgaclear
vgaclear
	adrl r0,wipesize
	ldr r0,[r0]
	mov r1,#0
	adrl r2,screenbase
	ldr r2,[r2]
clearthescreen
	str r1,[r2],#4
	subs r0,r0,#4
	bgt clearthescreen
  movs pc,r14


cgainten
vgainten
	stmfd r13!,{r14}
	adrl r1,palettestatus
	ldr r2,[r1]
	bic r2,r2,#1
	orr r2,r2,r0
	str r2,[r1]
	bl setpalette
	ldmfd r13!,{pc}


cgapal
vgapal
	stmfd r13!,{r14}
	adrl r1,palettestatus
	ldr r2,[r1]
	bic r2,r2,#2
	orr r2,r2,r0,asl #1
	str r2,[r1]
	bl setpalette
	ldmfd r13!,{pc}


setpalette
	stmfd r13!,{r4-r8,r14}
  adrl r0,vgacolours
	mov r1,#48
	mla r2,r1,r2,r0
	mov r4,#0
loop
	swi 256+19
	mov r0,r4
	swi 0x0
	swi 256+16
  ldrb r5,[r2],#1
	mov r0,r5,asl #2
	swi 0x0
	ldrb r5,[r2],#1
	mov r0,r5,asl #2
	swi 0x0
	ldrb r5,[r2],#1
	mov r0,r5,asl #2
	swi 0x0
  add r4,r4,#1
	cmp r4,#16
	blt loop
  ldmfd r13!,{r4-r8,pc}


cgaputi
vgaputi
; r0 is x position
; r1 is y position
; r2 is pointer to sprite data
; r3 is width of sprite
; ip[0] is height of sprite
  mov ip,sp
	stmdb sp!,{r4-r9,fp,ip,lr,pc}
	sub fp,ip,#4	; omit pc?

	ldr r4,[ip,#0]

	mov r4,r4,asl #1

	adrl r5,screenbase
	ldr r5,[r5]

	add r6,r5,r1,asl #9
	add r6,r6,r1,asl #7
	add r6,r6,r0

	bic r6,r6,#3

yplotloop1
	mov r9,r6
	mov r7,r3
xplotloop1
	ldr r8,[r2],#4
	subs r7,r7,#1
	str r8,[r9],#4
	bgt xplotloop1

	add r6,r6,#320
	subs r4,r4,#1
	bgt yplotloop1

	ldmdb fp,{r4-r9,fp,sp,pc}


; just copied from above mostly
cgageti
vgageti
	mov ip,sp
	stmdb sp!,{r4-r9,fp,ip,lr,pc}
	sub fp,ip,#4	; omit pc?

	ldr r4,[ip,#0]

	mov r4,r4,asl #1

	adrl r5,screenbase
	ldr r5,[r5]

	add r6,r5,r1,asl #9
	add r6,r6,r1,asl #7
	add r6,r6,r0

	bic r6,r6,#3

yplotloop2
	mov r9,r6
	mov r7,r3
xplotloop2
	ldr r8,[r9],#4
	subs r7,r7,#1
	str r8,[r2],#4
	bgt xplotloop2

	add r6,r6,#320
	subs r4,r4,#1
	bgt yplotloop2

	ldmdb fp,{r4-r9,fp,sp,pc}


cgaputim2
vgaputim2
	stmfd sp!,{r4-r11,r14}

	adrl r4,vgatable
	add r4,r4,r1,asl #3
	ldmia r4,{r5,r6}

	mov r3,r3,asl #1

	adrl r7,screenbase
	ldr r7,[r7]

	add r8,r7,r0		; top-left corner of sprite

yplotloop3
	mov r9,r2		; width
	mov r10,r8		; screen pointer
xplotloop3
	ldr r0,[r5],#4		; image data
	ldr r1,[r6],#4		; mask data

	ldr r11,[r10]

	and r11,r11,r1
	orr r11,r11,r0

	str r11,[r10],#4

	subs r9,r9,#1
	bgt xplotloop3

	add r8,r8,#320

	subs r3,r3,#1
	bgt yplotloop3

	ldmfd sp!,{r4-r11,pc}


cgagetpix
vgagetpix
	stmfd r13!,{r4-r6,r14}
	adrl r2,screenbase
	ldr r2,[r2]

	add r3,r2,r1,asl #9
	add r3,r3,r1,asl #7
	add r3,r3,r0

	ldrb r4,[r3]
	ldrb r5,[r3,#1]
	orr r4,r4,r5,asl #8
	ldrb r5,[r3,#2]
	orr r4,r4,r5,asl #16
	ldrb r5,[r3,#3]
	orr r4,r4,r5,asl #24

	add r3,r3,#320

	ldrb r5,[r3]
	ldrb r6,[r3,#1]
	orr r5,r5,r6,asl #8
	ldrb r6,[r3,#2]
	orr r5,r5,r6,asl #16
	ldrb r6,[r3,#3]
	orr r5,r5,r6,asl #24

	mov r6,#255

	tst r4,#15
	tsteq r5,#15
	biceq r6,r6,#128

	tst r4,#15<<4
	tsteq r5,#15<<4
	biceq r6,r6,#64

	tst r4,#15<<8
	tsteq r5,#15<<8
	biceq r6,r6,#32

	tst r4,#15<<12
	tsteq r5,#15<<12
	biceq r6,r6,#16

	tst r4,#15<<16
	tsteq r5,#15<<16
	biceq r6,r6,#8

	tst r4,#15<<20
	tsteq r5,#15<<20
	biceq r6,r6,#4

	tst r4,#15<<24
	tsteq r5,#15<<24
	biceq r6,r6,#2

	tst r4,#15<<28
	tsteq r5,#15<<28
	biceq r6,r6,#1

	and r0,r6,#&ee

	ldmfd r13!,{r4-r6,pc}


cgatitle
vgatitle
	stmfd r13!,{r4-r8,r14}

	adrl r0,vgatitledat
	adrl r1,screenbase
	ldr r1,[r1]
	mov r2,#3	; pseudo-bitplane
bitplaneloop
	mov r8,#128000
	mov r3,r1
writeloop
	ldrb r4,[r0],#1	; get byte from rle data
	cmp r4,#254	; does rle pair follow?
	ldreqb r7,[r0],#1
	ldreqb r4,[r0],#1
	movne r7,#1

	cmp r7,#0
	moveq r7,#256
	mov r6,#0

	tst r4,#128
	orrne r6,r6,#1
	tst r4,#64
	orrne r6,r6,#1<<4
	tst r4,#32
	orrne r6,r6,#1<<8
	tst r4,#16
	orrne r6,r6,#1<<12
	tst r4,#8
	orrne r6,r6,#1<<16
	tst r4,#4
	orrne r6,r6,#1<<20
	tst r4,#2
	orrne r6,r6,#1<<24
	tst r4,#1
	orrne r6,r6,#1<<28

	mov r6,r6,asl r2

goback
	ldr r5,[r3]	; get word from screen
	orr r5,r5,r6
	str r5,[r3],#4

	subs r8,r8,#4
	beq out

	subs r7,r7,#1
	bgt goback

	b writeloop
out
	subs r2,r2,#1
	bge bitplaneloop

	ldmfd r13!,{r4-r8,pc}


cgawrite
vgawrite
	stmfd r13!,{r4-r12,r14}

	adrl r4,screenbase
	ldr r4,[r4]

	add r5,r4,r1,asl #9
	add r5,r5,r1,asl #7
	add r5,r5,r0

	adrl r6,ascii2vga
	adrl r7,textoffdat

	add r0,r7,r3,asl #4

	sub r2,r2,#32
	cmp r2,#0x5f
	bgt dontplot
  ldr r2,[r6,r2,asl #2]

	mov r6,#0
characterplaneloop
	ldr r1,[r0,r6,asl #2]
	add r2,r2,r1

  mov r1,#(1<<3)+(1<<7)
	orr r1,r1,#(1<<11)+(1<<15)
	orr r1,r1,#(1<<19)+(1<<23)
	orr r1,r1,#(1<<27)+(1<<31)

	mov r12,#24
plotcharactery
	mov r9,r5
	mov r11,#3
plotcharacterx
	ldrb r7,[r2],#1

	mov r8,#0

	tst r7,#128
	orrne r8,r8,#1
	tst r7,#64
	orrne r8,r8,#1<<4
	tst r7,#32
	orrne r8,r8,#1<<8
	tst r7,#16
	orrne r8,r8,#1<<12
	tst r7,#8
	orrne r8,r8,#1<<16
	tst r7,#4
	orrne r8,r8,#1<<20
	tst r7,#2
	orrne r8,r8,#1<<24
	tst r7,#1
	orrne r8,r8,#1<<28

	rsb r10,r6,#3
	mov r8,r8,asl r10

	ldr r10,[r9]
	bic r10,r10,r1,lsr r6
	orr r10,r10,r8
	str r10,[r9],#4

	subs r11,r11,#1
	bgt plotcharacterx

	add r5,r5,#320

	subs r12,r12,#1
	bgt plotcharactery

	sub r5,r5,#320*24

	add r6,r6,#1
	cmp r6,#4
	blt characterplaneloop

dontplot
	ldmfd r13!,{r4-r12,pc}


	END
