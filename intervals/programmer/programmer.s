.global __do_clear_bss  ; This line causes the linker to include this routine to clear the .bss section.

; Timer 1 interrupt vector.

.global __vector_13
__vector_13:  ; TIMER1_OVF_vect
  push r0
  in r0, 0x3f
  push r0

  inc r4

  pop r0
  out 0x3f, r0
  pop r0
  reti

.global __vector_10
__vector_10:  ; TIMER1_CAPT_vect
  push r31
  push r30
  push r27
  push r26
  push r25
  push r24
  push r23
  push r22
  push r21
  push r20
  push r19
  push r18
  push r1
  push r0
  in r0, 0x3f
  push r0

  lds r5, 0x86
  lds r6, 0x87
  rcall sendTimerData

  pop r0
  out 0x3f, r0
  pop r0
  pop r1
  pop r18
  pop r19
  pop r20
  pop r21
  pop r22
  pop r23
  pop r24
  pop r25
  pop r26
  pop r27
  pop r30
  pop r31
  reti


; The toolchain links in code that at the start of the program, sets SP = 0x8ff (top of RAM) and the status flags all to 0 before calling main.

.global main
main:

  ; Initialize hardware ports

  ; DDRB value:   0x2e  (Port B Data Direction Register)
  ;   DDB0           0  Sync in                     - input
  ;   DDB1           2  Blue LED (OC1A)             - output
  ;   DDB2           4  Shift register reset        - output
  ;   DDB3           8  Shift register data (MOSI)  - output
  ;   DDB4           0  Escape switch               - input
  ;   DDB5        0x20  Shift register clock (SCK)  - output
  ;   DDB6           0  XTAL1/TOSC1
  ;   DDB7           0  XTAL2/TOSC2
  ldi r31, 0x2e
  out 0x04, r31

  ; PORTB value:  0x11  (Port B Data Register)
  ;   PORTB0         1  Sync in                     - pull-up enabled
  ;   PORTB1         0  Blue LED (OC1A)
  ;   PORTB2         0  Shift register reset        - low
  ;   PORTB3         0  Shift register data (MOSI)
  ;   PORTB4      0x10  Escape switch               - pull-up enabled
  ;   PORTB5         0  Shift register clock (SCK)
  ;   PORTB6         0  XTAL1/TOSC1
  ;   PORTB7         0  XTAL2/TOSC2
  ldi r31, 0x11
  out 0x05, r31

  ; TCCR0A value: 0xa3  (Timer/Counter 0 Control Register A)
  ;   WGM00          1  } Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
  ;   WGM01          2  }
  ;
  ;
  ;   COM0B0         0  } Compare Output Mode for Channel B: non-inverting mode
  ;   COM0B1      0x20  }
  ;   COM0A0         0  } Compare Output Mode for Channel A: non-inverting mode
  ;   COM0A1      0x80  }
  ldi r31, 0xa3
  out 0x24, r31

  ; TCCR0B value: 0x01  (Timer/Counter 0 Control Register B)
  ;   CS00           1  } Clock select: clkIO/1 (no prescaling)
  ;   CS01           0  }
  ;   CS02           0  }
  ;   WGM02          0  Waveform Generation Mode = 3 (Fast PWM, TOP=0xff)
  ;
  ;
  ;   FOC0B          0  Force Output Compare B
  ;   FOC0A          0  Force Output Compare A
  ldi r31, 0x01
  out 0x25, r31

  ; TIMSK0 value: 0x00  (Timer/Counter 0 Interrupt Mask Register)
  ;   TOIE0          0  Timer 0 overflow:  no interrupt
  ;   OCIE0A         0  Timer 0 compare A: no interrupt
  ;   OCIE0B         0  Timer 0 compare B: no interrupt
  ldi r31, 0x00
  sts 0x6e, r31

  ; TIMSK1 value: 0x21  (Timer/Counter 1 Interrupt Mask Register)
  ;   TOIE1          1  Timer 1 overflow:  interrupt
  ;   OCIE1A         0  Timer 1 compare A: no interrupt
  ;   OCIE1B         0  Timer 1 compare B: no interrupt
  ;
  ;
  ;   ICIE1       0x20  Timer 1 input capture: interrupt
  ldi r31, 0x00
  sts 0x6f, r31

  ; TIMSK2 value: 0x00  (Timer/Counter 2 Interrupt Mask Register)
  ;   TOIE2          0  Timer 2 overflow:  no interrupt
  ;   OCIE2A         0  Timer 2 compare A: no interrupt
  ;   OCIE2B         0  Timer 2 compare B: no interrupt
  ldi r31, 0x00
  sts 0x70, r31

  ; TCCR1A value: 0x00  (Timer/Counter 1 Control Register A)
  ;   WGM10          0  } Waveform Generation Mode = 0 (Normal)
  ;   WGM11          0  }
  ;
  ;
  ;   COM1B0         0  } Compare Output Mode for Channel B: normal port operation, OC1B disconnected
  ;   COM1B1         0  }
  ;   COM1A0         0  } Compare Output Mode for Channel A: normal port operation, OC1A disconnected
  ;   COM1A1         0  }
  ldi r31, 0x00
  sts 0x80, r31

  ; TCCR1B value: 0x01  (Timer/Counter 1 Control Register B)
  ;   CS10           1  } Clock select: clkIO/1 (no prescaling)
  ;   CS11           0  }
  ;   CS12           0  }
  ;   WGM12          0  } Waveform Generation Mode = 0 (Normal)
  ;   WGM13          0  }
  ;
  ;   ICES1          0  Input Capture Edge Select: falling
  ;   ICNC1          0  Input Capture Noise Canceler: disabled
  ldi r31, 0x01
  sts 0x81, r31

  ; TCCR1C value: 0x00  (Timer/Counter 1 Control Register C)
  ;
  ;
  ;
  ;
  ;
  ;
  ;   FOC1B          0  Force Output Compare for Channel B
  ;   FOC1A          0  Force Output Compare for Channel A
  ldi r31, 0x00
  sts 0x82, r31

  ; ICR1 value: 0x0400  (Timer/Counter 1 Input Capture Register)
  ;   Timer 1 overflow frequency (15.625KHz at 16MHz clock frequency)
  ldi r31, 0x04
  sts 0x87, r31
  ldi r31, 0x00
  sts 0x86, r31

  ; OCR1A value: blue LED brightness
  ldi r31, 0x00
  sts 0x89, r31
  sts 0x88, r31

  ; UCSR0A value: 0x00  (USART Control and Status Register 0 A)
  ;   MPCM0          0  Multi-processor Communcation Mode: disabled
  ;   U2X0           0  Double the USART Transmission Speed: disabled
  ;
  ;
  ;
  ;
  ;   TXC0           0  USART Transmit Complete: not cleared
  ldi r31, 0x00
  sts 0xc0, r31

  ; UCSR0B value: 0xb8  (USART Control and Status Register 0 B)
  ;   TXB80          0  Transmit Data Bit 8 0
  ;
  ;   UCSZ02         0  Character Size 0: 8 bit
  ;   TXEN0          8  Transmitter Enable 0: enabled
  ;   RXEN0       0x10  Receiver Enable 0: enabled
  ;   UDRIE0      0x20  USART Data Register Empty Interrupt Enable 0: disabled
  ;   TXCIE0         0  TX Complete Interrupt Enable 0: disabled
  ;   RXCIE0      0x80  RX Complete Interrupt Enable 0: disabled
  ldi r31, 0xb8
  sts 0xc1, r31

  ; UCSR0C value: 0x06  (USART Control and Status Register 0 C)
  ;   UCPOL0         0  Clock Polarity
  ;   UCSZ00         2  Character Size: 8 bit
  ;   UCSZ01         4  Character Size: 8 bit
  ;   USBS0          0  Stop Bit Select: 1-bit
  ;   UPM00          0  Parity Mode: disabled
  ;   UPM01          0  Parity Mode: disabled
  ;   UMSEL00        0  USART Mode Select: asynchronous
  ;   UMSEL01        0  USART Mode Select: asynchronous
  ldi r31, 0x06
  sts 0xc2, r31

  ; UBRR0L value: 0x67  (USART Baud Rate Register Low) - 9600bps
  ldi r31, 0x67
  sts 0xc4, r31

  ; UBRR0H value: 0x00  (USART Baud Rate Register High)
  ldi r31, 0x00
  sts 0xc5, r31

  sei  ; enable interrupts

idleLoop:
  jmp idleLoop


.global raiseVDD
raiseVDD:
  sbi 0x05, 3
  ret

.global lowerVDD
lowerVDD:
  cbi 0x05, 3
  ret

.global raiseVPP
raiseVPP:
  sbi 0x05, 2
  ret

.global lowerVPP
lowerVPP:
  cbi 0x05, 2
  ret

.global raiseClock
raiseClock:
  sbi 0x05, 1
  ret

.global lowerClock
lowerClock:
  cbi 0x05, 1
  ret

.global raiseData
raiseData:
  sbi 0x05, 0
  ret

.global lowerData
lowerData:
  cbi 0x05, 0
  ret

.global getData
getData:
  eor r12, r12
  sbic 0x03, 0
  inc r12
  ret

.global setDataInput
setDataInput:
  cbi 0x04, 0
  ret

.global setDataOutput
setDataOutput:
  sbi 0x04, 0
  ret

.global wait100ns
wait100ns:  ; 2 cycles
  ret

.global wait1us
wait1us:    ; 16 cycles
  ldi r31, 5         ; 1
wait1usLoop:
  dec r31            ; 5*1
  brne wait1usLoop   ; 4*2 + 1
  ret                ; 2

.global wait5us
wait5us:    ; 80 cycles
  ldi r30, 5
wait5usLoop:
  rcall wait1us
  dec r30
  brne wait5usLoop
  ret

.global wait100us
wait100us:  ; 1600 cycles
  ldi r27, 20
wait100usLoop:
  rcall wait5us
  dec r27
  brne wait100usLoop
  ret

.global wait2ms
wait2ms:    ; 32000 cycles
  ldi r26, 20
wait2msLoop:
  rcall wait100us
  dec r26
  brne wait2msLoop
  ret

.global wait10ms
wait10ms:   ; 160000 cycles
  ldi r25, 5
wait10msLoop:
  rcall wait2ms
  dec r25
  brne wait10msLoop
  ret

.global startTimer
startTimer:
  ldi r31, 0x21
  sts 0x6f, r31
  ret

.global stopTimer
stopTimer:
  ldi r31, 0x00
  sts 0x6f, r31
  ret

