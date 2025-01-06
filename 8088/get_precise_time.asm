; Returns a time counter with precision/resolution of 838ns and accuracy limited by interrupt overhead
; Assumes PIT channel 0 is running faster than 18.2Hz in mode 2 and that TimerDivisor does not change

    PROC    GetPreciseTime
    PUBLIC  GetPreciseTime

    mov cx,[TimerDivisor]
    cli
    mov al,0
    out 43h,al
    in al,40h
    mov bl,al
    in al,40h
    mov bh,al
    mov ax,[TimerCount]
    mov dx,[TimerCount+2]
    sti
    cmp ax,[TimerCount]     ; If timer wrapped before we read it we should have had an interrupt by now
    je @@noCorrection

    ; TimerCount changed, figure out if timer wrapped before or after we latched it
    sub bx,cx
    neg bx
    shr cx,1
    cmp bx,cx
    ja @@wrappedAfterLatch

    ; Wrapped before latch - add in an extra TimerDivisor worth of ticks
    add ax,bx
    adc dx,0
    add ax,[TimerDivisor]
    adc dx,0
    ret

@@noCorrection:
    sub bx,cx
    neg bx
@@wrappedAfterLatch:
    add ax,bx
    adc dx,0
    ret
    ENDP

