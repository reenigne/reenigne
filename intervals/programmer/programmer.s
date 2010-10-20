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
  push r0
  in r0, 0x3f
  push r0

  in r5, ICR1L
  in r6, ICR1H

  pop r0
  out 0x3f, r0
  pop r0
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

  ; DDRC value:   0x07  (Port C Data Direction Register)
  ;   DDC0           1  Volume potentiometer (ADC0) - input
  ;   DDC1           2  Tempo potentiometer (ADC1)  - input
  ;   DDC2           4  Tuning potentiometer (ADC2) - input
  ;   DDC3           0  Switch selector A           - output
  ;   DDC4           0  Switch selector B           - output
  ;   DDC5           0  Switch selector C           - output
  ;   DDC6           0  ~RESET
  ldi r31, 0x07
  out 0x07, r31

  ; PORTC value:  0x00  (Port C Data Register)
  ;   PORTC0         0  Volume potentiometer (ADC0)
  ;   PORTC1         0  Tempo potentiometer (ADC1)
  ;   PORTC2         0  Tuning potentiometer (ADC2)
  ;   PORTC3         0  Switch selector A           - low
  ;   PORTC4         0  Switch selector B           - low
  ;   PORTC5         0  Switch selector C           - low
  ;   PORTC6         0  ~RESET
  ldi r31, 0x00
  out 0x08, r31

  ; DDRD value:   0x6c  (Port D Data Direction Register)
  ;   DDD0           0  Debugging (RXD)
  ;   DDD1           0  Debugging (TXD)
  ;   DDD2           4  Sync out                    - output
  ;   DDD3           8  Audio output (OC2B)         - output
  ;   DDD4           0  Switch input 0              - input
  ;   DDD5        0x20  Red LED (OC0B)              - output
  ;   DDD6        0x40  Green LED (OC0A)            - output
  ;   DDD7           0  Switch input 1              - input
  ldi r31, 0x6c
  out 0x0a, r31

  ; PORTD value:  0x91  (Port D Data Register)
  ;   PORTD0         1  Debugging (RXD)             - pull-up enabled
  ;   PORTD1         0  Debugging (TXD)
  ;   PORTD2         0  Sync out                    - low
  ;   PORTD3         0  Audio output (OC2B)
  ;   PORTD4      0x10  Switch input 0              - pull-up enabled
  ;   PORTD5         0  Red LED (OC0B)
  ;   PORTD6         0  Green LED (OC0A)
  ;   PORTD7      0x80  Switch input 1              - pull-up enabled
  ldi r31, 0x91
  out 0x0b, r31

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
  ldi r31, 0x20
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


; Initialize registers
  eor r2, r2                      ; r2 = 0
  eor r4, r4                      ; r4 = 0

; Clear RAM.
  ldi r31, 1
  ldi r30, 0
clearLoop:
  st Z+, r2
  cpi r31, 9
  brne clearLoop

  sei  ; enable interrupts

  jmp idleLoop


raiseVDD:
  sbi 0x05, 3
  ret

lowerVDD:
  cbi 0x05, 3
  ret

raiseVPP:
  sbi 0x05, 2
  ret

lowerVPP:
  cbi 0x05, 2
  ret

raiseClock:
  sbi 0x05, 1
  ret

lowerClock:
  cbi 0x05, 1
  ret

raiseData:
  sbi 0x05, 0
  ret

lowerData:
  cbi 0x05, 0
  ret

getData:
  eor r12, r12
  sbic 0x03, 0
  inc r12
  ret

setDataInput:
  cbi 0x04, 0
  ret

setDataOuput:
  sbi 0x04, 0
  ret

wait80ns:
wait100ns:  ; 2 cycles
  ret

wait1us:    ; 16 cycles
  ldi r31, 5         ; 1
wait1usLoop:
  dec r31            ; 5*1
  brne wait1usLoop   ; 4*2 + 1
  ret                ; 2

wait5us:    ; 80 cycles
  ldi r30, 5
wait5usLoop:
  rcall wait1us
  dec r30
  brne wait5usLoop
  ret

wait100us:  ; 1600 cycles
  ldi r27, 20
wait100usLoop:
  rcall wait5us
  dec r27
  brne wait100usLoop
  ret

wait2ms:    ; 32000 cycles
  ldi r26, 20
wait2msLoop:
  rcall wait100us
  dec r26
  brne wait2msLoop
  ret

wait10ms:   ; 160000 cycles
  ldi r25, 5
wait10msLoop:
  rcall wait2ms
  dec r25
  brne wait10msLoop
  ret

