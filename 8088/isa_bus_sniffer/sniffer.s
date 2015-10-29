.global main

;SIGNAL(RESET_vect)
;{
;.global __vector_0
;__vector_0:   ; RESET_vect
main:

  ; Initialize hardware ports

  ; DDRB value:   0x07  (Port B Data Direction Register)
  ;   DDB0           1  Multiplexer select 0        - output
  ;   DDB1           2  Multiplexer select 1        - output
  ;   DDB2           4  Multiplexer select 2        - output
  ;   DDB3           0                              - input
  ;   DDB4           0                              - input
  ;   DDB5           0                              - input
  ;   DDB6           0  XTAL1/TOSC1
  ;   DDB7           0  XTAL2/TOSC2
  ldi r31, 0x07
  out 0x04, r31

  ; PORTB value:  0x04  (Port B Data Register)
  ;   PORTB0         0  Multiplexer select 0        - low
  ;   PORTB1         0  Multiplexer select 1        - low
  ;   PORTB2         4  Multiplexer select 2        - high
  ;   PORTB3         0
  ;   PORTB4         0
  ;   PORTB5         0
  ;   PORTB6         0  XTAL1/TOSC1
  ;   PORTB7         0  XTAL2/TOSC2
  ldi r31, 0x04
  out 0x05, r31

  ; DDRC value:   0x00  (Port C Data Direction Register)
  ;   DDC0           0  Sampling channel 0          - input
  ;   DDC1           0  Sampling channel 1          - input
  ;   DDC2           0  Sampling channel 2          - input
  ;   DDC3           0  Sampling channel 3          - input
  ;   DDC4           0  Sampling channel 4          - input
  ;   DDC5           0  Sampling channel 5          - input
  ;   DDC6           0  ~RESET
  ldi r31, 0x00
  out 0x07, r31

  ; PORTC value:  0x00  (Port C Data Register)
  ;   PORTC0         0  Sampling channel 0
  ;   PORTC1         0  Sampling channel 1
  ;   PORTC2         0  Sampling channel 2
  ;   PORTC3         0  Sampling channel 3
  ;   PORTC4         0  Sampling channel 4
  ;   PORTC5         0  Sampling channel 5
  ;   PORTC6         0  ~RESET
  ldi r31, 0x00
  out 0x08, r31

  ; DDRD value:   0x00  (Port D Data Direction Register)
  ;   DDD0           0  PC IO and sampling channel 6 - input
  ;   DDD1           0  PC IO
  ;   DDD2           0  Sampling channel 7           - input
  ;   DDD3           0  Sampling channel 8           - input
  ;   DDD4           0  Sampling channel 9           - input
  ;   DDD5           0  Sampling channel 10          - input
  ;   DDD6           0  Sampling channel 11          - input
  ;   DDD7           0  Sampling channel 12          - input
  ldi r31, 0x00
  out 0x0a, r31

  ; PORTD value:  0x00  (Port D Data Register)
  ;   PORTD0         0  PC IO and sampling channel 6 - input
  ;   PORTD1         0  PC IO
  ;   PORTD2         0  Sampling channel 7           - input
  ;   PORTD3         0  Sampling channel 8           - input
  ;   PORTD4         0  Sampling channel 9           - input
  ;   PORTD5         0  Sampling channel 10          - input
  ;   PORTD6         0  Sampling channel 11          - input
  ;   PORTD7         0  Sampling channel 12          - input
  ldi r31, 0x00
  out 0x0b, r31

  ; TCCR0A value: 0x00  (Timer/Counter 0 Control Register A)
  ;   WGM00          0  } Waveform Generation Mode = 0 (Normal)
  ;   WGM01          0  }
  ;
  ;
  ;   COM0B0         0  } Compare Output Mode for Channel B: Normal port operation, OC0A disconnected.
  ;   COM0B1         0  }
  ;   COM0A0         0  } Compare Output Mode for Channel A: Normal port operation, OC0B disconnected.
  ;   COM0A1         0  }
  ldi r31, 0x00
  out 0x24, r31

  ; TCCR0B value: 0x00  (Timer/Counter 0 Control Register B)
  ;   CS00           0  } Clock select: No clock source (Timer/Counter stopped))
  ;   CS01           0  }
  ;   CS02           0  }
  ;   WGM02          0  Waveform Generation Mode = 0 (Normal)
  ;
  ;
  ;   FOC0B          0  Force Output Compare B
  ;   FOC0A          0  Force Output Compare A
  ldi r31, 0x00
  out 0x25, r31

  ; TIMSK0 value: 0x00  (Timer/Counter 0 Interrupt Mask Register)
  ;   TOIE0          0  Timer 0 overflow:  no interrupt
  ;   OCIE0A         0  Timer 0 compare A: no interrupt
  ;   OCIE0B         0  Timer 0 compare B: no interrupt
  ldi r31, 0x00
  sts 0x6e, r31

  ; TCCR1A value: 0x00  (Timer/Counter 1 Control Register A)
  ;   WGM10          0  } Waveform Generation Mode = 0 (Normal)
  ;   WGM11          0  }
  ;
  ;
  ;   COM1B0         0  } Compare Output Mode for Channel B: Normal port operation, OC1A disconnected.
  ;   COM1B1         0  }
  ;   COM1A0         0  } Compare Output Mode for Channel A: Normal port operation, OC1B disconnected.
  ;   COM1A1         0  }
  ldi r31, 0x00
  sts 0x80, r31

  ; TCCR1B value: 0x00  (Timer/Counter 1 Control Register B)
  ;   CS10           0  } Clock select: No clock source (Timer/Counter stopped))
  ;   CS11           0  }
  ;   CS12           0  }
  ;   WGM12          0  } Waveform Generation Mode = 0 (Normal)
  ;   WGM13          0  }
  ;
  ;   ICES1          0  Input Capture Edge Select: falling
  ;   ICNC1          0  Input Capture Noise Canceler: disabled
  ldi r31, 0x00
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

  ; TIMSK1 value: 0x00  (Timer/Counter 1 Interrupt Mask Register)
  ;   TOIE1          0  Timer 1 overflow:  no interrupt
  ;   OCIE1A         0  Timer 1 compare A: no interrupt
  ;   OCIE1B         0  Timer 1 compare B: no interrupt
  ldi r31, 0x00
  sts 0x6f, r31

  ; TCCR2A value: 0x00  (Timer/Counter 2 Control Register A)
  ;   WGM20          0  } Waveform Generation Mode = 0 (Normal)
  ;   WGM21          0  }
  ;
  ;
  ;   COM2B0         0  } Compare Output Mode for Channel B: Normal port operation, OC2A disconnected.
  ;   COM2B1         0  }
  ;   COM2A0         0  } Compare Output Mode for Channel A: Normal port operation, OC2B disconnected.
  ;   COM2A1         0  }
  ldi r31, 0x00
  sts 0xb0, r31

  ; TCCR2B value: 0x00  (Timer/Counter 2 Control Register B)
  ;   CS20           0  } Clock select: No clock source (Timer/Counter stopped))
  ;   CS21           0  }
  ;   CS22           0  }
  ;   WGM22          0  Waveform Generation Mode = 0 (Normal)
  ;
  ;
  ;   FOC2B          0  Force Output Compare B
  ;   FOC2A          0  Force Output Compare A
  ldi r31, 0x00
  sts 0xb1, r31

  ; TIMSK2 value: 0x00  (Timer/Counter 2 Interrupt Mask Register)
  ;   TOIE2          0  Timer 2 overflow:  no interrupt
  ;   OCIE2A         0  Timer 2 compare A: no interrupt
  ;   OCIE2B         0  Timer 2 compare B: no interrupt
  ldi r31, 0x00
  sts 0x70, r31

  ; EICRA value:  0x00  (External Interrupt Control Register A)
  ;   ISC00          0  } The low level of INT0 generates an interrupt request.
  ;   ISC01          0  }
  ;   ISC10          0  } The low level of INT1 generates an interrupt request.
  ;   ISC11          0  }
  ldi r31, 0x00
  sts 0x69, r31

  ; EIMSK value:  0x00  (External Interrupt Mask Register)
  ;   INT0           0
  ;   INT1           0
  ldi r31, 0x00
  out 0x1d, r31

  ; PCICR value:  0x02  (Pin Change Interrupt Control Register)
  ;   PCIE0          0  Pin Change Interrupt Enable 0: disabled
  ;   PCIE1          2  Pin Change Interrupt Enable 1: enabled
  ;   PCIE2          0  Pin Change Interrupt Enable 2: disabled
  ldi r31, 0x02
  sts 0x68, r31

  ; PCMSK0 value: 0x00  (Pin Change Mask Register 0)
  ;   PCINT0         0  Pin Change Enable Mask 0: disabled
  ;   PCINT1         0  Pin Change Enable Mask 1: disabled
  ;   PCINT2         0  Pin Change Enable Mask 2: disabled
  ;   PCINT3         0  Pin Change Enable Mask 3: disabled
  ;   PCINT4         0  Pin Change Enable Mask 4: disabled
  ;   PCINT5         0  Pin Change Enable Mask 5: disabled
  ;   PCINT6         0  Pin Change Enable Mask 6: disabled
  ;   PCINT7         0  Pin Change Enable Mask 7: disabled
  ldi r31, 0x00
  sts 0x6b, r31

  ; PCMSK1 value: 0x02  (Pin Change Mask Register 1)
  ;   PCINT8         0  Pin Change Enable Mask 8: disabled
  ;   PCINT9         2  Pin Change Enable Mask 9: enabled
  ;   PCINT10        0  Pin Change Enable Mask 10: disabled
  ;   PCINT11        0  Pin Change Enable Mask 11: disabled
  ;   PCINT12        0  Pin Change Enable Mask 12: disabled
  ;   PCINT13        0  Pin Change Enable Mask 13: disabled
  ;   PCINT14        0  Pin Change Enable Mask 14: disabled
  ldi r31, 0x02
  sts 0x6c, r31

  ; PCMSK2 value: 0x00  (Pin Change Mask Register 2)
  ;   PCINT16        0  Pin Change Enable Mask 16: disabled
  ;   PCINT17        0  Pin Change Enable Mask 17: disabled
  ;   PCINT18        0  Pin Change Enable Mask 18: disabled
  ;   PCINT19        0  Pin Change Enable Mask 19: disabled
  ;   PCINT20        0  Pin Change Enable Mask 20: disabled
  ;   PCINT21        0  Pin Change Enable Mask 21: disabled
  ;   PCINT22        0  Pin Change Enable Mask 22: disabled
  ;   PCINT23        0  Pin Change Enable Mask 23: disabled
  ldi r31, 0x00
  sts 0x6d, r31

  ; SPCR value:   0x00  (SPI Control Register)
  ;   SPR0           0  } SPI Clock Rate Select: fOSC/4
  ;   SPR1           0  }
  ;   CPHA           0  Clock Phase: sample on leading edge, setup on trailing edge
  ;   CPOL           0  Clock Polarity: leading edge = rising, trailing edge = falling
  ;   MSTR           0  Master/Slave Select: slave
  ;   DORD           0  Data Order: MSB first
  ;   SPE            0  SPI Enable: disabled
  ;   SPIE           0  SPI Interrupt Enable: disabled
  ldi r31, 0x00
  out 0x2c, r31

  ; SPSR value:   0x00  (SPI Status Register)
  ;  SPI2X           0  SPI Clock Rate Select: fOSC/4
  ldi r31, 0x00
  out 0x2d, r31

  ; UCSR0A value: 0x02  (USART Control and Status Register 0 A)
  ;   MPCM0          0  Multi-processor Communcation Mode: disabled
  ;   U2X0           2  Double the USART Transmission Speed: enabled
  ;
  ;
  ;
  ;
  ;   TXC0           0  USART Transmit Complete: not cleared
  ldi r31, 0x02
  sts 0xc0, r31

  ; UCSR0B value: 0xd8  (USART Control and Status Register 0 B)
  ;   TXB80          0  Transmit Data Bit 8 0
  ;
  ;   UCSZ02         0  Character Size 0: 8 bit
  ;   TXEN0          8  Transmitter Enable 0: enabled
  ;   RXEN0          0  Receiver Enable 0: disabled
  ;   UDRIE0         0  USART Data Register Empty Interrupt Enable 0: disabled
  ;   TXCIE0         0  TX Complete Interrupt Enable 0: disabled
  ;   RXCIE0         0  RX Complete Interrupt Enable 0: disabled
  ldi r31, 0x08
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

  ; UBRR0L value: 0x0f  (USART Baud Rate Register Low) - 115200bps  (actually 111861, error = 2.8%)
  ldi r31, 0x0f
  sts 0xc4, r31

  ; UBRR0H value: 0x00  (USART Baud Rate Register High)
  ldi r31, 0x00
  sts 0xc5, r31

  ; SMCR value:   0x01  (Sleep Mode Control Register)
  ;   SE             1  Sleep enabled
  ;   SM0            0  } Sleep Mode = idle
  ;   SM1            0  }
  ;   SM2            0  }
  ldi r31, 0x01
  out 0x33, r31

  ; MCUCR value:  0x00  (MCU Control Register)
  ;   IVCE           0
  ;   IVSEL          0
  ;
  ;
  ;   PUD            0
  ;   BODSE          0
  ;   BOSD           0
  ldi r31, 0x00
  out 0x35, r31


.macro waitUART
1:
  lds r27, 0xc0
  sbrs r27, 5
  rjmp 1b
.endm


  waitUART
  ldi r31,43    ; '+'
  sts 0xc6,r31

  waitUART
  ldi r31,45    ; '-'
  sts 0xc6,r31

  cli

  ; Main loop
mainLoop:
  ; Set up stack:
  ldi r31, 0xff        ; 21 28 27 32
  out 0x3d, r31        ; 22 29 28 33
  ldi r31, 0x08        ; 23 30 29 34
  out 0x3e, r31        ; 24 31 30 35
  sei                  ; 25 32 31 36

;waitForNoSignal:
;  in r17, 0x06   ; C
;  sbrc r17, 1
;  rjmp waitForNoSignal
;
;waitForSignal:
;  in r17, 0x06       ; 0 5 7  ; C  Total interrupt latency to this point should be 9 hdots, well within a single IO access of 12.
;  in r16, 0x09       ; 1      ; D
;
;  sbrs r17, 1        ; 2
;  rjmp waitForSignal ;   3
;  sbrc r17, 0        ; 4
;  rjmp waitForSignal ;     5
                     ; 6

  sleep                ; 26 33 32 37
;sloop:
;  rjmp sloop

  rjmp mainLoop

.macro run port, count
  .rept \count
    in r0, \port
    st Y+, r0
  .endr
.endm

;.global __vector_3
;__vector_3:   ; PCINT0_vect
;  waitUART
;  ldi r31,'A'
;  sts 0xc6,r31
;  rjmp mainLoop
;
;.global __vector_5
;__vector_5:   ; PCINT1_vect
;  waitUART
;  ldi r31,'B'
;  sts 0xc6,r31
;  rjmp mainLoop



;SIGNAL(PCINT1_vect)
;{
.global __vector_4
__vector_4:   ; PCINT1_vect
  ldi r31, 2
  out 0x1b, r31

  ; Pin C1 (address line A19) changed.
  ; First we check if A19==1, A15==0 (address 0xN0000..0xN7fff where N==8..F) - these are the only addresses we respond to.
  in r16, 0x09   ; D
  in r17, 0x06   ; C  Total interrupt latency to this point should be 9 hdots, well within a single IO access of 12.

;  waitUART
;  ldi r31,'C'
;  sts 0xc6,r31

  andi r17, 3          ;  9
  cpi r17, 2           ; 10
  brne mainLoop        ; 11

waitForSignalToStop:
  in r17, 0x06
  andi r17, 3
  cpi r17, 2
  breq waitForSignalToStop

  andi r16, 0xe0       ; 13

;  ldi r31,49 ; '1'
;  sts 0xc6,r31

  ; Look at address bits A11, A7 and A3 to determine command.
  cpi r16, 0           ; 15
  brne notStart        ; 16

  ; Start of command load sequence
  ldi r30, 0           ; 17
  ldi r31, 1           ; 18
  rjmp mainLoop        ; 19

notStart:
  cpi r16, 0x80        ;    18
  brne notData0        ;    19

  ldi r16, 0           ;    20
  st Z+, r16           ;    21
  rjmp checkEndData    ;    23

notData0:
  cpi r16, 0xc0        ;       24
  brne mainLoop        ;       25

  ldi r16, 1           ;          26
  st Z+, r16           ;          27

checkEndData:
  cpi r30, 40          ;    25    29
  brne mainLoop        ;    26    30

  ; Pack bits
  ldi r25, 0
.rept 8
  lsl r25
  ld r29, -Z
  or r25, r29
.endr

  ldi r24, 0
.rept 8
  lsl r24
  ld r29, -Z
  or r24, r29
.endr

  ldi r18, 0
.rept 8
  lsl r18
  ld r29, -Z
  or r18, r29
.endr

  ldi r17, 0
.rept 8
  lsl r17
  ld r29, -Z
  or r17, r29
.endr

  ldi r16, 0
.rept 8
  lsl r16
  ld r29, -Z
  or r16, r29
.endr

  ; Delay for N hdots (N is in bytes 3 and 4 of the request data) while we're
  ; getting DRAM refresh going again.
  mov r23, r24
  lsr r25
  ror r24
  lsr r25
  ror r24
waitLoop:
  sbiw r24, 1
  brne waitLoop
  andi r23, 3
  cpi r23, 1
  brge delay1a
delay1a:
  cpi r23, 2
  brge delay2a
delay2a:
  cpi r23, 3
  brge delay3a
delay3a:


  ; Set up multiplexer channel bits
  mov r20, r16
  ldi r21, 7
  and r20, r21
  out 0x05, r20

  ; Set up Z (jump address)
  ldi r30, lo8(readC + 8188)
  ldi r31, hi8(readC + 8188)
  sbrc r16, 3
  ldi r30, lo8(readD + 8188)
  sbrc r16, 3
  ldi r31, hi8(readD + 8188)
  lsr r31
  ror r30

  mov r29, r18                                       ; 0x07
  mov r28, r17                                       ; 0xff
  andi r29,7
  add r28, r28                                       ; 0xfe
  adc r29, r29                                       ; 0x0f
  sub r30, r28
  sbc r31, r29

  ; Delay for 0-2 cycles
  mov r29, r16
  swap r29
  andi r29, 3
  cpi r29, 1
  brge delay1
delay1:
  cpi r29, 2
  brge delay2
delay2:

  ; Do the actual aquisition
  ldi r28, 0
  ldi r29, 1
  ijmp
readC:
  run 0x06, 2048
  jmp doSend
readD:
  run 0x09, 2048

  ; Send the data back
doSend:
  mov r31, r18                                       ; 0x07
  mov r30, r17                                       ; 0xff
  andi r31, 7
  adiw r30, 1                                        ; 0x800

  ldi r28, 0
  ldi r29, 1

  mov r19, r16
  ori r19, 0x80
  waitUART
  sts 0xc6, r19

  mov r19, r17
  andi r19, 0x7f
  waitUART
  sts 0xc6, r19

  mov r19, r17
  rol r19
  mov r19, r18
  rol r19
  andi r19, 0x7f
  waitUART
  sts 0xc6, r19

sendLoop:
  ld r19, Y+

  sbrs r16, 3
  rjmp noRotate
  mov r20, r19
  ror r20  ; C <- bit 0
  ror r19  ; 07654321
  ror r19  ; 10765432

noRotate:
  andi r19, 0x7f
  waitUART
  sts 0xc6, r19
  sbiw r30, 1
  brne sendLoop

  ; Set the multiplexer channel back to the correct one for our address lines
  ldi r31, 0x04
  out 0x05, r31

  ; Clear any spurious interrupts
  ldi r31, 2
  out 0x1b, r31

  jmp mainLoop


;//SIGNAL(USART_TX_vect)
;//{
;.global __vector_20
;__vector_20:  ; USART_TX_vect


;SIGNAL(USART_RX_vect)
;{
;.global __vector_18
;__vector_18:  ; USART_RX_vect
;  ; Get a byte from serial and treat it as a command from the XT
;  lds r16, 0xc6
;  jmp signalled





;signalled:
;    if (r16 == 0x00) {
;        Z = 0x100;
;        goto mainLoop;
;    }
;    if (r16 == 0x80) {
;        *(Z++) = 0;
;        if (Z == endZ)
;            goto doSampleAndSend;
;        goto mainLoop;
;    }
;    if (r16 == 0xc0) {
;        *(Z++) = 1;
;        if (Z == endZ)
;            goto doSampleAndSend;
;        goto mainLoop;
;    }
;    goto mainLoop;
;
;doSampleAndSend:
;    Byte byte0 = 0;
;    Byte byte1 = 0;
;    Byte byte2 = 0;
;    for (int i = 0; i < 8; ++i) {
;        byte0 |= (memory[0x100 + i] << i);
;        byte1 |= (memory[0x108 + i] << i);
;        byte2 |= (memory[0x110 + i] << i);
;    }
;    int length = ((byte1 | (byte2 << 8)) & 0x7ff) + 1;
;
;    // Set up channel bits
;    out(5, byte0 & 7);
;
;    Z = ((byte0 & 8) != 0 ? readC : readD) + (2048 - length)*2;
;
;    cycleDelay((byte0 >> 4) & 2);
;
;    Y = 0x100;
;
;    ijmp
;
;doSend:
;    for (int i = 0; i < length; ++i) {
;        while ((UCSR0A & 0x20)==0);  // 0x20 == UDRE0, [0xc0] == UCSR0A
;        UDR0 = memory[i + 0x100];                 // [0xc6] = UDR0
;    }
;
;    out(5, 4);
;    goto mainLoop;




;     (0xC6) UDR0                                                                     196                   USART 0 I/O Data Register                         -
;     (0xC5) UBRR0H                                                                   200                   USART 0 Baud Rate Register High                   *
;     (0xC4) UBRR0L                                                                   200                   USART 0 Baud Rate Register Low                    *
;     (0xC2) UCSR0C   UMSEL01 UMSEL00 UPM01   UPM00   USBS0   UCSZ01/UDORD0 UCSZ00/UCPHA0 UCPOL0 198/213    USART 0 Control and Status Register C             *
;     (0xC1) UCSR0B   RXCIE0  TXCIE0  UDRIE0  RXEN0   TXEN0   UCSZ02  RXB80   TXB80   197                   USART 0 Control and Status Register B             *
;     (0xC0) UCSR0A   RXC0    TXC0    UDRE0   FE0     DOR0    UPE0    U2X0    MPCM0   196                   USART 0 Control and Status Register A             *
;     (0xBD) TWAMR    TWAM6   TWAM5   TWAM4   TWAM3   TWAM2   TWAM1   TWAM0   -       245                   TWI (Slave) Address Mask Register
;     (0xBC) TWCR     TWINT   TWEA    TWSTA   TWSTO   TWWC    TWEN    -       TWIE    242                   TWI Control Register
;     (0xBB) TWDR                                                                     244                   TWI Data Register
;     (0xBA) TWAR     TWA6    TWA5    TWA4    TWA3    TWA2    TWA1    TWA0    TWGCE   245                   TWI (Slave) Address Register
;     (0xB9) TWSR     TWS7    TWS6    TWS5    TWS4    TWS3    -       TWPS1   TWPS0   244                   TWI Status Register
;     (0xB8) TWBR                                                                     242                   TWI Bit Rate Register
;     (0xB6) ASSR     -       EXCLK   AS2     TCN2UB  OCR2AUB OCR2BUB TCR2AUB TCR2BUB 165                   Asynchronous Status Register
;     (0xB4) OCR2B                                                                    163                   Timer/Counter 2 Output Compare Register B         -
;     (0xB3) OCR2A                                                                    163                   Timer/Counter 2 Output Compare Register A         -
;     (0xB2) TCNT2                                                                    163                   Timer/Counter 2 Register                          -
;     (0xB1) TCCR2B   FOC2A   FOC2B   -       -       WGM22   CS22    CS21    CS20    162                   Timer/Counter 2 Control Register B                *
;     (0xB0) TCCR2A   COM2A1  COM2A0  COM2B1  COM2B0  -       -       WGM21   WGM20   159                   Timer/Counter 2 Control Register A                *
;     (0x8B) OCR1BH                                                                   139                   Timer/Counter 1 Output Compare Register B High    -
;     (0x8A) OCR1BL                                                                   139                   Timer/Counter 1 Output Compare Register B Low     -
;     (0x89) OCR1AH                                                                   139                   Timer/Counter 1 Output Compare Register A High    -
;     (0x88) OCR1AL                                                                   139                   Timer/Counter 1 Output Compare Register A Low     -
;     (0x87) ICR1H                                                                    139                   Timer/Counter 1 Input Capture Register High       -
;     (0x86) ICR1L                                                                    139                   Timer/Counter 1 Input Capture Register Low        -
;     (0x85) TCNT1H                                                                   139                   Timer/Counter 1 Register High                     -
;     (0x84) TCNT1L                                                                   139                   Timer/Counter 1 Register Low                      -
;     (0x82) TCCR1C   FOC1A   FOC1B   -       -       -       -       -       -       138                   Timer/Counter 1 Control Register C                *
;     (0x81) TCCR1B   ICNC1   ICES1   -       WGM13   WGM12   CS12    CS11    CS10    137                   Timer/Counter 1 Control Register B                *
;     (0x80) TCCR1A   COM1A1  COM1A0  COM1B1  COM1B0  -       -       WGM11   WGM10   135                   Timer/Counter 1 Control Register A                *
;     (0x7F) DIDR1    -       -       -       -       -       -       AIN1D   AIN0D   250                   Digital Input Disable Register 1
;     (0x7E) DIDR0    -       -       ADC5D   ADC4D   ADC3D   ADC2D   ADC1D   ADC0D   267                   Digital Input Disable Register 0
;     (0x7C) ADMUX    REFS1   REFS0   ADLAR   -       MUX3    MUX2    MUX1    MUX0    263                   ADC Multiplexer Selection Register
;     (0x7B) ADCSRB   -       ACME    -       -       -       ADTS2   ADTS1   ADTS0   266                   ADC Control and Status Register B
;     (0x7A) ADCSRA   ADEN    ADSC    ADATE   ADIF    ADIE    ADPS2   ADPS1   ADPS0   264                   ADC Control and Status Register A
;     (0x79) ADCH                                                                     266                   ADC Data Register High byte
;     (0x78) ADCL                                                                     266                   ADC Data Register Low byte
;     (0x70) TIMSK2   -       -       -       -       -       OCIE2B  OCIE2A  TOIE2   164                   Timer/Counter 2 Interrupt Mask Register           *
;     (0x6F) TIMSK1   -       -       ICIE1   -       -       OCIE1B  OCIE1A  TOIE1   140                   Timer/Counter 1 Interrupt Mask Register           *
;     (0x6E) TIMSK0   -       -       -       -       -       OCIE0B  OCIE0A  TOIE0   112                   Timer/Counter 0 Interrupt Mask Register           *
;     (0x6D) PCMSK2   PCINT23 PCINT22 PCINT21 PCINT20 PCINT19 PCINT18 PCINT17 PCINT16  75                   Pin Change Mask Register 2                        *
;     (0x6C) PCMSK1   -       PCINT14 PCINT13 PCINT12 PCINT11 PCINT10 PCINT9  PCINT8   75                   Pin Change Mask Register 1                        *
;     (0x6B) PCMSK0   PCINT7  PCINT6  PCINT5  PCINT4  PCINT3  PCINT2  PCINT1  PCINT0   75                   Pin Change Mask Register 0                        *
;     (0x69) EICRA    -       -       -       -       ISC11   ISC10   ISC01   ISC00    72                   External Interrupt Control Register A             *
;     (0x68) PCICR    -       -       -       -       -       PCIE2   PCIE1   PCIE0    74                   Pin Change Interrupt Control Register             *
;     (0x66) OSCCAL                                                                    37                   Oscillator Calibration Register
;     (0x64) PRR      PRTWI   PRTIM2  PRTIM0  -       PRTIM1  PRSPI   PRUSART0 PRADC   42                   Power Reduction Register
;     (0x61) CLKPR    CLKPCE  -       -       -       CLKPS3  CLKPS2  CLKPS1  CLKPS0   37                   Clock Prescale Register
;     (0x60) WDTCSR   WDIF    WDIE    WDP3    WDCE    WDE     WDP2    WDP1    WDP0     55                   Watchdog Timer Control Register
;0x3F (0x5F) SREG     I       T       H       S       V       N       Z       C         9                   AVR Status Register
;0x3E (0x5E) SPH      -       -       -       -       -       SP10    SP9     SP8      12                   Stack Pointer High
;0x3D (0x5D) SPL      SP7     SP6     SP5     SP4     SP3     SP2     SP1     SP0      12                   Stack Pointer Low
;0x37 (0x57) SPMCSR   SPMIE   RWWSB   -       RWWSRE  BLBSET  PGWRT   PGERS   SELFPRGEN 294                 Store Program Memory Control and Status Register
;0x35 (0x55) MCUCR    -       BODS    BODSE   PUD     -       -       IVSEL   IVCE     45/69/93             MCU Control Register                              *
;0x34 (0x54) MCUSR    -       -       -       -       WDRF    BORF    EXTRF   PORF     55                   MCU Status Register
;0x33 (0x53) SMCR     -       -       -       -       SM2     SM1     SM0     SE       40                   Sleep Mode Control Register                       *
;0x30 (0x50) ACSR     ACD     ACBG    ACO     ACI     ACIE    ACIC    ACIS1   ACIS0   248                   Analog Comparator Control and Status Register
;0x2E (0x4E) SPDR                                                                     176                   SPI Data Register                                 -
;0x2D (0x4D) SPSR     SPIF    WCOL    -       -       -       -       -       SPI2X   175                   SPI Status Register                               *
;0x2C (0x4C) SPCR     SPIE    SPE     DORD    MSTR    CPOL    CPHA    SPR1    SPR0    174                   SPI Control Register                              *
;0x2B (0x4B) GPIOR2                                                                    25                   General Purpose I/O Register 2
;0x2A (0x4A) GPIOR1                                                                    25                   General Purpose I/O Register 1
;0x28 (0x48) OCR0B                                                                                          Timer/Counter 0 Output Compare Register B         -
;0x27 (0x47) OCR0A                                                                                          Timer/Counter 0 Output Compare Register A         -
;0x26 (0x46) TCNT0                                                                                          Timer/Counter 0 Register                          -
;0x25 (0x45) TCCR0B   FOC0A   FOC0B   -       -       WGM02   CS02    CS01    CS00                          Timer/Counter 0 Control Register B                *
;0x24 (0x44) TCCR0A   COM0A1  COM0A0  COM0B1  COM0B0  -       -       WGM01   WGM00                         Timer/Counter 0 Control Register A                *
;0x23 (0x43) GTCCR    TSM     -       -       -       -       -       PSRASY  PSRSYNC 144/166               General Timer/Counter Control Register
;0x22 (0x42) EEARH                                                                     21                   EEPROM Address Register High
;0x21 (0x41) EEARL                                                                     21                   EEPROM Address Register Low
;0x20 (0x40) EEDR                                                                      21                   EEPROM Data Register
;0x1F (0x3F) EECR     -       -       EEPM1   EEPM0   EERIE   EEMPE   EEPE    EERE     21                   EEPROM Control Register
;0x1E (0x3E) GPIOR0                                                                    25                   General Purpose I/O Register 0
;0x1D (0x3D) EIMSK    -       -       -       -       -       -       INT1    INT0     73                   External Interrupt Mask Register                  *
;0x1C (0x3C) EIFR     -       -       -       -       -       -       INTF1   INTF0    73                   External Interrupt Flag Register                  -
;0x1B (0x3B) PCIFR    -       -       -       -       -       PCIF2   PCIF1   PCIF0    74                   Pin Change Interrupt Flag Register                -
;0x17 (0x37) TIFR2    -       -       -       -       -       OCF2B   OCF2A   TOV2    164                   Timer/Counter 2 Interrupt Flag Register           -
;0x16 (0x36) TIFR1    -       -       ICF1    -       -       OCF1B   OCF1A   TOV1    140                   Timer/Counter 1 Interrupt Flag Register           -
;0x15 (0x35) TIFR0    -       -       -       -       -       OCF0B   OCF0A   TOV0    112                   Timer/Counter 0 Interrupt Flag Register           -
;0x0B (0x2B) PORTD    PORTD7  PORTD6  PORTD5  PORTD4  PORTD3  PORTD2  PORTD1  PORTD0   94                   Port D Data Register                              *
;0x0A (0x2A) DDRD     DDD7    DDD6    DDD5    DDD4    DDD3    DDD2    DDD1    DDD0     94                   Port D Data Direction Register                    *
;0x09 (0x29) PIND     PIND7   PIND6   PIND5   PIND4   PIND3   PIND2   PIND1   PIND0    94                   Port D Input Pins Address                         -
;0x08 (0x28) PORTC    -       PORTC6  PORTC5  PORTC4  PORTC3  PORTC2  PORTC1  PORTC0   93                   Port C Data Register                              *
;0x07 (0x27) DDRC     -       DDC6    DDC5    DDC4    DDC3    DDC2    DDC1    DDC0     93                   Port C Data Direction Register                    *
;0x06 (0x26) PINC     -       PINC6   PINC5   PINC4   PINC3   PINC2   PINC1   PINC0    93                   Port C Input Pins Address                         -
;0x05 (0x25) PORTB    PORTB7  PORTB6  PORTB5  PORTB4  PORTB3  PORTB2  PORTB1  PORTB0   93                   Port B Data Register                              *
;0x04 (0x24) DDRB     DDB7    DDB6    DDB5    DDB4    DDB3    DDB2    DDB1    DDB0     93                   Port B Data Direction Register                    *
;0x03 (0x23) PINB     PINB7   PINB6   PINB5   PINB4   PINB3   PINB2   PINB1   PINB0    93                   Port B Input Pins Address                         -

