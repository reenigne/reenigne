%macro createRegister 1
  %assign valueOf_%1 0
  %assign symbolValueOf_%1 -1
  %assign isSplitRegister_%1 0
  %assign isSplitByteRegister_%1 0
  %assign isLowByteRegister_%1 0
  %assign isGeneralPurposeWordRegister_%1 0
  %assign isSegmentRegister_%1 0
  %assign isPortRegister_%1 0
%endmacro

%assign wordRegisterIndex 0
%assign symbolIndex 0

%macro createSymbol 1
  %assign symbolIndex_%1 symbolIndex
  %assign symbolName_%[symbolIndex] %1
  %assign symbolIndex symbolIndex+1
%endmacro

%macro createWordRegister 1
  createRegister %1
  %assign isWordRegister_%1 1
  %assign registerNumber_%1 wordRegisterIndex
  %define registerName_%[wordRegisterIndex] %1
  %assign wordRegisterIndex wordRegisterIndex+1
%endmacro

%macro createGeneralPurposeWordRegister 1
  createWordRegister %1
  %assign isGeneralPurposeWordRegister_%1 1
%endmacro

%macro createSegmentRegister 1
  createWordRegister %1
  %assign isSegmentRegister_%1 1
%endmacro

%macro createByteRegister 1
  createRegister %1
  %assign isWordRegister_%1 0
%endmacro

%macro createSplitByteRegister 3
  createByteRegister %1
  %assign isSplitByteRegister_%1 1
  %assign isLowByteRegister_%1 %2
  %define containingRegisterOf_%1 %3
%endmacro

%macro createSplitRegister 1
  createGeneralPurposeWordRegister %1x
  createSplitByteRegister %1l,0,%1x
  createSplitByteRegister %1h,1,%1x
  %define lowByteOf_%1x %1l
  %define highByteOf_%1x %1h
  %assign isSplitRegister_%1x 1
%endmacro

%macro createPortRegister 2
  createByteRegister %1
  %assign registerPort_%1 %2
  %assign isPortRegister_%1 1
  %assign isIndexedPortRegister_%1 0
%endmacro

createSplitRegister a
createSplitRegister b
createSplitRegister c
createSplitRegister d
createGeneralPurposeWordRegister si
createGeneralPurposeWordRegister di
createGeneralPurposeWordRegister sp
createGeneralPurposeWordRegister bp
createSegmentRegister es
createSegmentRegister ds
createSegmentRegister ss
createPortRegister cgaCrtcIndex, 0x3d4

%macro createCRTCPortRegister 2
  createPortRegister cga_%1, 0x3d5
  %assign isIndexedPortRegister_cga_%1 1
  %define indexRegister_cga_%1 cgaCrtcIndex
  %assign indexValue_cga_%1 %2
%endmacro

createCRTCPortRegister horizontalTotal, 0
createCRTCPortRegister horizontalDisplayed, 1
createCRTCPortRegister horizontalSyncPosition, 2
createCRTCPortRegister horizontalSyncWidth, 3
createCRTCPortRegister verticalTotal, 4
createCRTCPortRegister verticalTotalAdjust, 5
createCRTCPortRegister verticalDisplayed, 6
createCRTCPortRegister verticalSyncPosition, 7
createCRTCPortRegister interlaceMode, 8
createCRTCPortRegister maximumScanLineAddress, 9
createCRTCPortRegister cursorStart, 10
createCRTCPortRegister cursorEnd, 11
createCRTCPortRegister startAddressHigh, 12
createCRTCPortRegister startAddressLow, 13
createCRTCPortRegister cursorHigh, 14
createCRTCPortRegister cursorLow, 15
createPortRegister cgaMode, 0x3d8
createPortRegister cgaPalette, 0x3d9
createPortRegister pitChannel0, 0x40
createPortRegister pitChannel1, 0x41
createPortRegister pitChannel2, 0x42
createPortRegister pitMode, 0x43

%assign queuedPortWrite 0
%assign queuedWritePortAddress 0
%assign queuedWritePortValue 0

; This just allows us to write multiple instructions on one source line.
%macro multi 1-*.nolist
  %rep %0
    %1
    %rotate 1
  %endrep
%endmacro

%macro twice 1.nolist
  multi %1, %1
%endmacro

%macro addConstantHelper 3.nolist
  %if isGeneralPurposeWordRegister_%1
    twice {%3 %1}
  %else
    add %1, %2
  %endif
%endmacro

%macro addConstant 2.nolist
  %if %2==2
    addConstantHelper %1, %2, inc
  %elif %2==-2
    addConstantHelper %1, %2, dec
  %elif %2==1
    inc %1
  %elif %2==-1
    dec %1
  %elif %2!=0
    add %1, %2
  %endif
%endmacro

%macro setHelper 2
  %assign symbolValueOf_%1 -1
  %assign valueOf_%1 %2
%endmacro

%macro setSymbolHelper 3
  %assign symbolValueOf_%1 symbolIndex_%3
  %assign valueOf_%1 %2
%endmacro


; Sets a register to a constant value
%macro set4 2
  %if valueOf_%1 != %2 || symbolValueOf_%1 != 0
    mov %1, %2
    setHelper %1, %2
    %if isSplitRegister_%1
      setHelper lowByteOf_%1, (%2) & 0xff
      setHelper highByteOf_%1, (%2) >> 8
    %elif isSplitByteRegister_%1
      %if isLowByteRegister_%1
        %if symbolValueOf_%[highByteOf_%[containingRegisterOf_%1]] == 0
          setHelper containingRegisterOf_%1, (%2) | (valueOf_%[highByteOf_%[containingRegisterOf_%1]] << 8)
        %endif
      %else
        %if symbolValueOf_%[lowByteOf_%[containingRegisterOf_%1]] == 0
          setHelper containingRegisterOf_%1, (%2 << 8) | (valueOf_%[lowByteOf_%[containingRegisterOf_%1]])
        %endif
      %endif
    %endif
  %endif
%endmacro

; Helper for set to avoid macro recursion.
%macro set3 2
  %if symbolValueOf_%1 != 0 || valueOf_%1 != %2
    %assign done 0
    %if isGeneralPurposeWordRegister_%1
      %if symbolValueOf_%1 == 0 && valueOf_%1 == (%2) + 1
        dec %1
        %assign done 1
      %elif symbolValueOf_%1 == 0 && valueOf_%1 == (%2) - 1
        inc %1
        %assign done 1
      %elif %2 == 0
        xor %1, %1
        %assign done 1
      %elif symbolValueOf_%1 == 0 && valueOf_%1 == (%2) + 2
        twice {dec %1}
        %assign done 1
      %elif symbolValueOf_%1 == 0 && valueOf_%1 == (%2) - 2
        twice {inc %1}
        %assign done 1
      %endif
    %endif
    %if isWordRegister_%1 && !done
      %assign i 0
      %rep wordRegisterIndex
        %if i != registerNumber_%1 && valueOf_%[registerName_%[i]] == %2
          mov %1, registerName_%[i]
          %assign done 1
          %exitrep
        %endif
        %assign i i+1
      %endrep
    %endif
    %if isSplitByteRegister_%1 && isLowByteRegister_%1 && symbolValueOf_%1 == 0 && !done
      %if valueOf_%1 == (%2) + 1 && valueOf_%1 != 0
        dec containingRegisterOf_%1
        %assign done 1
      %elif valueOf_%1 == (%2) - 1 && valueOf_%1 != 0xff
        inc containingRegisterOf_%1
        %assign done 1
      %endif
    %endif
    %if isSplitRegister_%1 && !done
      %if symbolValueOf_%[lowByteOf_%1] == 0 && valueOf_%[lowByteOf_%1] == ((%2) & 0xff)
        set4 highByteOf_%1, (%2) >> 8
      %else
        %if symbolValueOf_%[highByteOf_%1] == 0 && valueOf_%[highByteOf_%1] == ((%2) >> 8)
          set4 lowByteOf_%1, (%2) & 0xff
        %else
          set4 %1, %2
        %endif
      %endif
      %assign done 1
    %endif
    %if done
      setHelper %1, %2
    %else
      set4 %1, %2
    %endif
  %endif
%endmacro

%macro outputPortByte 1  ; data assumed in al
  %if %1 > 0xff || (symbolValueOf_dx == 0 && valueOf_dx == %1)
    set3 dx, %1
    out dx, al
  %else
    out %1, al
  %endif
%endmacro

%macro outputPortByte 2
  %if %1 > 0xff || (symbolValueOf_dx == 0 && valueOf_dx == %1)
    set3 dx, %1
    set3 al, %2
    out dx, al
  %else
    set3 al, %2
    out %1, al
  %endif
%endmacro

%macro outputPortWord 2
  %if %1 > 0xff
    set3 dx, %1
    set3 ax, %2
    out dx, ax
  %else
    set3 ax, %2
    out %1, ax
  %endif
%endmacro

%macro flushQueuedPortWrite 0
  %if queuedPortWrite
    outputPortByte queuedWritePortAddress, queuedWritePortValue
    %assign queuedPortWrite 0
  %endif
%endmacro

%macro queuePortWrite2 2
  %assign queuedWritePortAddress %1
  %assign queuedWritePortValue %2
  %assign queuedPortWrite 1
%endmacro

%macro queuePortWrite 2
  %if queuedPortWrite
    %if queuedWritePortAddress == %1 - 1
      outputPortWord queuedWritePortAddress, queuedWritePortValue | ((%2) << 8)
      %assign queuedPortWrite 0
      %assign done 1
    %else
      flushQueuedPortWrite
      queuePortWrite2 %1, %2
    %endif
  %else
    queuePortWrite2 %1, %2
  %endif
%endmacro

%macro set2 2
  %if symbolValueOf_%1 != 0 || valueOf_%1 != %2
    %assign done 0
    %if isPortRegister_%1
      queuePortWrite registerPort_%1, %2
      %assign done 1
    %endif
    %if isWordRegister_%1 && !done
      %assign i 0
      %rep wordRegisterIndex
        %if i != registerNumber_%1 && valueOf_%[i] == %2
          mov %1, registerName_%[i]
          %assign done 1
          %exitrep
        %endif
        %assign i i+1
      %endrep
    %elif isSegmentRegister_%1 && !done
      set3 ax, %2
      mov %1, ax
      %assign done 1
    %endif
    %if done
      setHelper %1, %2
    %else
      set3 %1, %2
    %endif
  %endif
%endmacro

; Sets a register to a constant value, performing various optimizations
%macro set 2
  %if symbolValueOf_%1 != 0 || valueOf_%1 != %2
    %if isPortRegister_%1
      %if isIndexedPortRegister_%1
        set2 indexRegister_%1, indexValue_%1
      %endif
      queuePortWrite registerPort_%1, %2
      setHelper %1, %2
    %else
      set2 %1, %2
    %endif
  %endif
%endmacro

%macro clear 1
  %assign symbolValueOf_%1 -1
  %if isSplitRegister_%1
    %assign symbolValueOf_%[lowByteOf_%1] -1
    %assign symbolValueOf_%[highByteOf_%1] -1
  %elif isSplitByteRegister_%1
    %assign symbolValueOf_%[containingRegisterOf_%1] -1
  %endif
%endmacro

%assign port_CGA_status 0x3da

; Timing note: With refresh off, these routines take 37 cycles per loop, then 27 cycles for the final test

%macro waitForDisplayEnable 0
  set dx, port_CGA_status
  %%wait:
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jnz %%wait                     ; 2 0 2   jump if -DISPEN, finish if +DISPEN
  clear al
%endmacro

%macro waitForDisplayDisable 0
  set dx, port_CGA_status
  %%wait:
    in al,dx                       ; 1 1 2
    test al,1                      ; 2 0 2
    jz %%wait                      ; 2 0 2   jump if not -DISPEN, finish if -DISPEN
  clear al
%endmacro

%macro waitForVerticalSync 0
  set dx, port_CGA_status
  %%wait:
    in al,dx
    test al,8
    jz %%wait                      ;         jump if not +VSYNC, finish if +VSYNC
  clear al
%endmacro

%macro waitForNoVerticalSync 0
  set dx, port_CGA_status
  %%wait:
    in al,dx
    test al,8
    jnz %%wait                     ;         jump if +VSYNC, finish if -VSYNC
  clear al
%endmacro

%macro setCGAmode 1-*
  %assign mode 8
  %assign palette 0
  %assign gotMode 0
  %assign gotPalette 0
  %rep %0
    %ifidn %1, hres
      %assign mode mode|1
      %assign gotMode 1
    %elifidn %1, grph
      %assign mode mode|2
      %assign gotMode 1
    %elifidn %1, bw
      %assign mode mode|4
      %assign gotMode 1
    %elifidn %1, video_disable
      %assign mode mode&(~8)
      %assign gotMode 1
    %elifidn %1, video_enable
      %assign mode mode|8
      %assign gotMode 1
    %elifidn %1, oneBpp
      %assign mode mode|0x10
      %assign gotMode 1
    %elifidn %1, enable_blink
      %assign mode mode|0x20
      %assign gotMode 1
    %else
      %assign palette %1
      %assign gotPalette 1
    %endif
  %endrep
  %if gotMode
    set cgaMode, mode
  %endif
  %if gotPalette
    set cgaPalette, palette
  %endif
  flushQueuedPortWrite
%endmacro

%macro setCGACRTC 0-13  ; scanlines per row, rows displayed, columns displayed, vsyncRow, hsyncColumn, vertical total scanlines, horizontal total, start address, cursor address, cursor start, cursor end, interlace mode, hsync width
  %if (valueOf_cgaMode & 2)
    %assign scanlinesPerRow 2
  %else
    %assign scanlinesPerRow 8
  %endif
  %if %0 >= 1
    %assign scanlinesPerRow (%1)
  %endif
  %assign rowsDisplayed 200/scanlinesPerRow
  %if %0 >= 2
    %assign rowsDisplayed (%2)
  %endif
  %if (valueOf_cgaMode & 1)
    %assign columnsDisplayed 80
    %assign hsyncColumn 90
    %assign horizontalTotal 114
    %assign hsyncWidth 15
  %else
    %assign columnsDisplayed 40
    %assign hsyncColumn 45
    %assign horizontalTotal 57
    %assign hsyncWidth 10
  %endif
  %if %0 >= 3
    %assign columnsDisplayed (%3)
  %endif
  %assign vsyncRow 224/scanlinesPerRow
  %if %0 >= 4
    %assign vsyncRow (%4)
  %endif
  %if %0 >= 5
    %assign hsyncColumn (%5)
  %endif
  %assign verticalTotalScanlines 262
  %if %0 >= 6
    %assign verticalTotalScanlines (%6)
  %endif
  %if %0 >= 7
    %assign horizontalTotal (%7)
  %endif
  %assign startAddress 0
  %if %0 >= 8
    %assign startAddress (%8)
  %endif
  %assign cursorAddress 0x3fff
  %if %0 >= 9
    %assign cursorAddress (%9)
  %endif
  %assign cursorStart 6
  %if %0 >= 10
    %assign cursorStart (%10)
  %endif
  %assign cursorEnd 7
  %if %0 >= 11
    %assign cursorEnd (%11)
  %endif
  %assign interlaceMode 0
  %if %0 >= 12
    %assign interlaceMode (%12)
  %endif
  %if %0 >= 13
    %assign hsyncWidth (%13)
  %endif
  %assign nonAdjustRows verticalTotalScanlines/scanlinesPerRow
  %if nonAdjustRows > 128
    %assign nonAdjustRows 128
  %endif
  set cga_horizontalTotal, horizontalTotal - 1
  set cga_horizontalDisplayed, columnsDisplayed
  set cga_horizontalSyncPosition, hsyncColumn
  set cga_horizontalSyncWidth, hsyncWidth
  set cga_verticalTotal, nonAdjustRows - 1
  set cga_verticalTotalAdjust, (verticalTotalScanlines - nonAdjustRows*scanlinesPerRow)
  set cga_verticalDisplayed, rowsDisplayed
  set cga_verticalSyncPosition, vsyncRow
  set cga_interlaceMode, interlaceMode
  set cga_maximumScanLineAddress, scanlinesPerRow - 1
  set cga_cursorStart, cursorStart
  set cga_cursorEnd, cursorEnd
  set cga_startAddressHigh, (startAddress >> 8)
  set cga_startAddressLow, (startAddress & 0xff)
  set cga_cursorHigh, (cursorAddress >> 8)
  set cga_cursorLow, (cursorAddress & 0xff)
%endmacro

%macro setPITmode 0-4 ; channel, mode, bytes, bcd/binary
  %assign channel 0
  %if %0 >= 1
    %assign channel %1
  %endif
  %assign mode 2
  %if %0 >= 2
    %ifidn %2, interruptOnTerminalCount
      %assign mode 0
    %elifidn %2, programmableOneShot
      %assign mode 1
    %elifidn %2, rateGenerator
      %assign mode 2
    %elifidn %2, squareWaveGenerator
      %assign mode 3
    %elifidn %2, softwareTriggeredStrobe
      %assign mode 4
    %elifidn %2, hardwareTriggeredStrobe
      %assign mode 5
    %else
      %error Unknown PIT mode %2
    %endif
  %endif
  %assign bytes 3
  %if %0 >= 3
    %ifidn %3, latch
      %assign bytes 0
    %elifidn %3, lsb
      %assign bytes 1
    %elifidn %3, msb
      %assign bytes 2
    %elifidn %3, both
      %assign bytes 3
    %else
      %error Unknown PIT bytes %3
    %endif
  %endif
  %assign bcd 0
  %if %0 >= 4
    %ifidn %4, binary
      %assign bcd 0
    %elifidn %4, bcd
      %assign bcd 1
    %else
      %error Unknown PIT base %3
    %endif
  %endif
  set pitMode, channel*0x40 + bytes*0x10 + mode*2 + bcd
%endmacro

%macro writePIT16 2  ; channel, value
  set pitChannel%1, (%2) & 0xff
  set pitChannel%1, (%2) >> 8
%endmacro

%macro writePIT16 1  ; channel
  outputPortByte registerPort_pitChannel%1
  mov al,ah
  clear al
  outputPortByte registerPort_pitChannel%1
%endmacro

%macro setPIT16 3  ; channel, mode, value
  setPITmode %1, %2
  writePIT16 %1, %3
%endmacro

%macro setPIT16nonConstant 3  ; channel, mode, value
  setPITmode %1, %2
  mov ax,%3
  clear ax
  writePIT16 %1
%endmacro

%macro readPIT16 1
  setPITmode %1, 0, latch
  in al,0x40 + %1
  mov ah,al
  in al,0x40 + %1
  xchg ah,al
  clear ax
%endmacro


; TODO: printing
; TODO: interrupts
; TODO: optimized flag-setting
; TODO: cbw/cwd optimization
; TODO: xchg rl,rh optimization
; TODO: neg/not rw optimization
; TODO: shr/shl/rol/ror optimization
; TODO: add/sub/and/or/xor rw,rw optimization
; TODO: add symbolic values (e.g. codeSegment) that can be loaded as well as constants
