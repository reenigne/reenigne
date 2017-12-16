o18ed:
  sub bp,0x0384
  rcl bl,1
  sub di,0x0200
  rcl bl,1
  sub sp,0x10b8
  rcl bl,1
  sub si,0x085c
  rcl bl,1
  mov al,[bx]         ; 3
  mov bl,[es:bx]      ; 4
  out dx,al           ; 2
  loop o18ed



o18ed:
  sub bp,0x0384
  rcr ax,1
  sub di,0x0200
  rcr ax,1
  sub dx,0x10b8
  rcr ax,1
  sub bx,0x085c
  rcr ax,1
  xchg ax,si          ; 1
  lodsw               ; 3
  out 0x42,al         ; 3
  loop o18ed

;     +++++++----- Instrument and instrument phase (
;     |||||||
; fedcba9876543210
; ||||       ||||+- Bit 4 of last PIT period
; ||||       |||+-- Bit 5 of last PIT period
; ||||       ||+--- 0
; ||||       |+---- 0
; ||||       +----- Repeat of bit 4 of last PIT period to separate aligned and misaligned data
; |||+------------- Channel 0 pulsed
; ||+-------------- Channel 1 pulsed
; |+--------------- Channel 2 pulsed
; +---------------- Channel 3 pulsed

;
;  00 LSB (previous PIT period 01-0f)
;  01 MSB (previous PIT period 01-0f)
;  02 LSB (previous PIT period 20-2f)
;  03 MSB (previous PIT period 20-2f)
;  04-10 unusable (13 bytes)
;  11 LSB (previous PIT period 10-1f)
;  12 MSB (previous PIT period 10-1f)
;  13 LSB (previous PIT period 30-3f)
;  14 MSB (previous PIT period 30-3f)
;  15-1f unusable (11 bytes)

