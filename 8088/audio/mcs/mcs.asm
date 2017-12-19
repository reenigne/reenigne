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

; With 15 NOPs, 219 cycles raw
; 4 refreshes per iteration at period 15 = 60 PIT cycles = 240 CPU cycles = 19.89kHz


o18ed:            ; raw count 147 cycles, does not stabilize with refresh
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

; Sound port 0x42:
;
;     +++++++----- Instrument and instrument phase
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

; Sound port 0x61:
;
;     ++++++++---- Instrument and instrument phase
;     ||||||||
; fedcba9876543210
; ||||        |||+- Bit 4 of last port 0x61 value (0)
; ||||        ||+-- Bit 5 of last port 0x61 value (0)
; ||||        |+--- Bit 6 of last port 0x61 value (1)
; ||||        +---- Bit 7 of last port 0x61 value (0)
; |||+------------- Channel 0 pulsed
; ||+-------------- Channel 1 pulsed
; |+--------------- Channel 2 pulsed
; +---------------- Channel 3 pulsed
;
;  00-03 unusable (4 bytes)
;  04 LSB
;  05 MSB
;  06-0f unusuable (10 bytes)
;
; Possible distribution of bits:
;   3 bits: Remaining samples in pulse
;   2 bits: channel 0 volume
;   1 bit: channel 1 volume
;   1 bit: channel 2 volume
;   1 bit: channel 3 volume

; With 15 NOPs, 207 cycles raw
; 4 refreshes per iteration at period 14 = 56 PIT cycles = 224 CPU cycles = 21.31kHz

; With 14 NOPs, 203 cycles raw
; 3 refreshes per iteration at period 18 = 54 PIT cycles = 216 CPU cycles = 22.10kHz



o18ed:            ; raw count 143 cycles, 2 refreshes per iteration at period 19 = 38 PIT cycles = 152 CPU cycles = 31.40kHz
  sub bp,0x0384
  rcl al,1
  sub di,0x0200
  rcl al,1
  sub dx,0x10b8
  rcl al,1
  sub bx,0x085c
  rcl al,1
  xchg ax,si
  lodsb
  out 0x42,al
  loop o18ed

; With 15 NOPs, 203 cycles raw
; 3 refreshes per iteration at period 18 = 54 PIT cycles = 216 CPU cycles = 22.10kHz
; However, this routine has an issue with ah being stomped with the high byte of the patch address

  pop si
  pop word[si]
  mov cl,99    ; Count until next patch
  jmp mixPatch

