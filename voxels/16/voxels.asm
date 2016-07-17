;	Static Name Aliases
;
;	$S892_tau	EQU	tau
	TITLE   voxels.cpp
	.8087
INCLUDELIB      SLIBCE
INCLUDELIB	OLDNAMES.LIB
_TEXT	SEGMENT  WORD PUBLIC 'CODE'
_TEXT	ENDS
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	CONST, _BSS, _DATA
	ASSUME DS: DGROUP, SS: DGROUP
EXTRN	__acrtused:ABS
EXTRN	__fltused:ABS
EXTRN	__aNCIsin:NEAR
EXTRN	__aNCIcos:NEAR
EXTRN	_int86:NEAR
EXTRN	__aNftol:NEAR
CONST      SEGMENT
	ORG	$+20
$S892_tau	DQ	0401921fb54442d18r    ;	6.283185307179586
	ORG	$-28
$T991	DQ	03f70000000000000r    ;	3.906250000000000E-03
$T992	DD	03b4ccccdr   ;	3.1250000E-03
$T993	DQ	03fd0000000000000r    ;	.2500000000000000
CONST      ENDS
_TEXT      SEGMENT
	ASSUME	CS: _TEXT
	PUBLIC	_main
_main	PROC NEAR
; Line 8
	push	bp
	mov	bp,sp
	sub	sp,66	;0042H
	push	di
	push	si
;	regs = -52
;	rn = -38
;	yy = -4
;	d = -32
;	v = -36
;	register di = i
;	xf = -14
;	yf = -10
;	phase = -24
;	amplitude = -28
;	y = -42
;	register si = x
;	xp = -22
;	x = -18
;	maxY = -12
;	theta = -26
;	r = -14
;	xh = -8
;	yh = -10
;	h = -62
;	y = -16
;	yy = -20
; Line 10
	mov	WORD PTR [bp-52],19	;0013H	;regs
; Line 11
	lea	ax,WORD PTR [bp-52]	;regs
	push	ax
	push	ax
	mov	ax,16	;0010H
	push	ax
	call	_int86
	add	sp,6
; Line 13
	mov	WORD PTR [bp-38],256	;0100H	;rn
; Line 14
	mov	WORD PTR [bp-32],0	;d
	mov	WORD PTR [bp-30],-28672	;9000H
; Line 15
	mov	WORD PTR [bp-36],0	;v
	mov	WORD PTR [bp-34],-24576	;a000H
; Line 18
	xor	di,di
	mov	WORD PTR [bp-6],0
	mov	WORD PTR [bp-4],1	;yy
	xor	si,si
	mov	WORD PTR [bp-2],1
	neg	si
	neg	si
	adc	WORD PTR [bp-2],-1	;ffffH
	mov	cx,WORD PTR [bp-2]
$F913:
; Line 19
	mov	bx,di
	xor	ax,ax
	mov	dx,-24576	;a000H
	mov	es,dx
	add	bx,ax
	mov	ax,di
	mov	BYTE PTR es:[bx],al
; Line 18
	inc	di
	dec	si
	jne	$F913
	dec	cx
	jns	$F913
; Line 19
	mov	WORD PTR [bp-2],0
	mov	ax,-32640	;8080H
	xor	cx,cx
	xor	bx,bx
	mov	si,-28672	;9000H
	push	si
	mov	di,bx
	pop	es
	shr	cx,1
	rep	stosw
	jae	$L1017
	stosb
$L1017:
; Line 23
	mov	dx,1
	mov	bx,4
$F920:
; Line 24
	mov	di,1
	mov	cx,bx
	mov	WORD PTR [bp-18],bx	;x
	mov	WORD PTR [bp-20],bx	;yy
	mov	WORD PTR [bp-14],dx	;xf
$F924:
	fldz	
	fldz	
	mov	si,WORD PTR [bp-38]	;rn
; Line 25
	lodsb
	sub	ah,ah
	sub	dx,dx
	mov	WORD PTR [bp-54],0
	mov	WORD PTR [bp-56],0
	mov	WORD PTR [bp-58],dx
	mov	WORD PTR [bp-60],ax
	fild	QWORD PTR [bp-60]
	fstp	ST(2)
	lodsb
	sub	ah,ah
	mov	WORD PTR [bp-60],ax
	fild	WORD PTR [bp-60]
	mov	WORD PTR [bp-62],cx	;h
	fidiv	WORD PTR [bp-62]	;h
	fstp	ST(1)
	mov	WORD PTR [bp-6],0
	mov	WORD PTR [bp-4],-28672	;9000H	;yy
	mov	WORD PTR [bp-8],0	;xh
	mov	WORD PTR [bp-12],256	;0100H	;maxY
	mov	WORD PTR [bp-16],cx	;y
	mov	WORD PTR [bp-10],di	;yf
	mov	WORD PTR [bp-38],si	;rn
	fld	ST(1)
	fstp	DWORD PTR [bp-24]	;phase
	fst	DWORD PTR [bp-28]	;amplitude
	fwait	
; Line 27
$F931:
; Line 28
	xor	si,si
	mov	ax,WORD PTR [bp-8]	;xh
	mov	WORD PTR [bp-2],ax
$F935:
; Line 29
	les	bx,DWORD PTR [bp-6]
	sub	ah,ah
	mov	al,BYTE PTR es:[bx][si]
	sub	dx,dx
	mov	WORD PTR [bp-54],0
	mov	WORD PTR [bp-56],0
	mov	WORD PTR [bp-58],dx
	mov	WORD PTR [bp-60],ax
	fild	QWORD PTR [bp-60]
	fild	WORD PTR [bp-2]
	fadd	ST(0),ST(3)
	fmul	QWORD PTR $T991
	fmul	QWORD PTR $S892_tau
	call	__aNCIsin
	fld	ST(2)
	fmulp	ST(1),ST(0)
	faddp	ST(1),ST(0)
	call	__aNftol
	les	bx,DWORD PTR [bp-6]
	mov	BYTE PTR es:[bx][si],al
	mov	ax,WORD PTR [bp-14]	;xf
	add	WORD PTR [bp-2],ax
	inc	si
	cmp	si,256	;0100H
	jl	$F935
; Line 27
	add	BYTE PTR [bp-5],1
	mov	ax,WORD PTR [bp-10]	;yf
	add	WORD PTR [bp-8],ax	;xh
	dec	WORD PTR [bp-12]	;maxY
	jne	$F931
; Line 24
	fstp	ST(0)
	fstp	ST(0)
	mov	cx,WORD PTR [bp-16]	;y
	add	cx,WORD PTR [bp-18]	;x
	mov	di,ax
	inc	di
	cmp	di,25	;0019H
	jge	$JCC373
	jmp	$F924
$JCC373:
; Line 23
	mov	dx,WORD PTR [bp-14]	;xf
	inc	dx
	mov	bx,WORD PTR [bp-20]	;yy
	add	bx,4
	cmp	bx,100	;0064H
	jge	$JCC391
	jmp	$F920
$JCC391:
; Line 33
	xor	bx,bx
; Line 34
$D939:
; Line 35
	mov	WORD PTR [bp-18],0	;x
	mov	WORD PTR [bp-22],bx	;xp
$F943:
; Line 36
	mov	WORD PTR [bp-12],0	;maxY
; Line 37
	mov	ax,WORD PTR [bp-18]	;x
	add	ax,WORD PTR [bp-22]	;xp
	mov	WORD PTR [bp-62],ax	;h
	fild	WORD PTR [bp-62]	;h
	fmul	DWORD PTR $T992
	fmul	QWORD PTR $T993
	fmul	QWORD PTR $S892_tau
	fstp	DWORD PTR [bp-26]	;theta
	fwait	
; Line 38
	mov	WORD PTR [bp-14],1	;xf
$F949:
	fldz	
	fldz	
; Line 39
	fld	DWORD PTR [bp-26]	;theta
	fst	ST(2)
	call	__aNCIsin
	fild	WORD PTR [bp-14]	;xf
	fst	ST(2)
	fmulp	ST(1),ST(0)
	call	__aNftol
	mov	si,ax
	add	si,WORD PTR [bp-22]	;xp
	and	si,255	;00ffH
; Line 40
	fld	ST(1)
	call	__aNCIcos
	fmul	ST(0),ST(1)
	call	__aNftol
	sub	ah,ah
	mov	WORD PTR [bp-10],ax	;yf
; Line 42
	mov	cx,25	;0019H
	mov	bx,ax
	mov	bh,bl
	sub	bl,bl
	add	bx,si
	mov	di,-28672	;9000H
	mov	es,di
	xor	di,di
	mov	al,BYTE PTR es:[bx][di]
	sub	ah,ah
	sub	ax,256	;0100H
	imul	cx
	cwd	
	idiv	WORD PTR [bp-14]	;xf
	add	ax,180	;00b4H
	mov	WORD PTR [bp-16],ax	;y
; Line 43
	mov	ax,WORD PTR [bp-12]	;maxY
	mov	WORD PTR [bp-20],ax	;yy
	mov	cx,WORD PTR [bp-16]	;y
	fstp	ST(0)
	fstp	ST(0)
	cmp	cx,ax
	jl	$FB962
	mov	ax,-320	;fec0H
	imul	WORD PTR [bp-20]	;yy
	add	ax,WORD PTR [bp-18]	;x
	add	ax,-1856	;f8c0H
	mov	dx,-24576	;a000H
	mov	WORD PTR [bp-4],ax	;yy
	mov	WORD PTR [bp-2],dx
	mov	ax,si
	sub	al,128	;0080H
	mov	ah,BYTE PTR [bp-10]	;yf
	mov	dx,es
	mov	WORD PTR [bp-66],ax
	mov	WORD PTR [bp-64],es
	mov	ax,cx
	sub	ax,WORD PTR [bp-20]	;yy
	inc	ax
	mov	WORD PTR [bp-6],ax
	mov	WORD PTR [bp-8],si	;xh
	lds	si,DWORD PTR [bp-4]	;yy
	ASSUME DS: NOTHING
	mov	di,ax
	mov	es,WORD PTR [bp-64]
$F960:
; Line 44
	mov	bx,WORD PTR [bp-66]
	mov	al,BYTE PTR es:[bx]
	mov	BYTE PTR [si],al
	add	si,-320	;fec0H
	dec	di
	jne	$F960
	push	ss
	pop	ds
	ASSUME DS: DGROUP
$FB962:
; Line 45
	mov	ax,WORD PTR [bp-12]	;maxY
	cmp	WORD PTR [bp-16],ax	;y
	jle	$FC950
; Line 46
	mov	ax,WORD PTR [bp-16]	;y
	mov	WORD PTR [bp-12],ax	;maxY
; Line 38
$FC950:
	inc	WORD PTR [bp-14]	;xf
	cmp	WORD PTR [bp-14],100	;0064H	;xf
	jge	$JCC656
	jmp	$F949
$JCC656:
; Line 48
	mov	di,ax
	cmp	di,200	;00c8H
	jge	$FC944
	mov	cx,-320	;fec0H
	imul	cx
	add	ax,WORD PTR [bp-18]	;x
	add	ax,-1856	;f8c0H
	mov	dx,-24576	;a000H
	mov	bx,ax
	mov	ds,dx
	ASSUME DS: NOTHING
	mov	ax,200	;00c8H
	sub	ax,di
	mov	WORD PTR [bp-2],ax
	mov	WORD PTR [bp-4],di	;yy
	mov	cx,ax
$F965:
; Line 49
	mov	BYTE PTR [bx],0
	add	bx,-320	;fec0H
	dec	cx
	jne	$F965
; Line 35
$FC944:
	push	ss
	pop	ds
	ASSUME DS: DGROUP
	inc	WORD PTR [bp-18]	;x
	cmp	WORD PTR [bp-18],320	;0140H	;x
	jge	$JCC720
	jmp	$F943
$JCC720:
; Line 50
	mov	bx,WORD PTR [bp-22]	;xp
; Line 51
	inc	bx
; Line 52
	jmp	$D939

_main	ENDP
_TEXT	ENDS
END
