void setup()
{
  pinMode(9, OUTPUT);
  
  // TCCR1A value: 0x82  (Timer/Counter 1 Control Register A)
  //   WGM10          0  } Waveform Generation Mode = 14 (Fast PWM, TOP=ICR1)
  //   WGM11          2  }
  //
  //
  //   COM1B0         0  } Compare Output Mode for Channel B: normal port operation, OC1B disconnected
  //   COM1B1         0  }
  //   COM1A0         0  } Compare Output Mode for Channel A: non-inverting mode
  //   COM1A1      0x80  }
  TCCR1A = 0x82;

  // TCCR1B value: 0x1b  (Timer/Counter 1 Control Register B)
  //   CS10           1  } Clock select: clkIO/64 (from prescaler)
  //   CS11           2  }
  //   CS12           0  }
  //   WGM12          8  } Waveform Generation Mode = 14 (Fast PWM, TOP=ICR1)
  //   WGM13       0x10  }
  //
  //   ICES1          0  Input Capture Edge Select: falling
  //   ICNC1          0  Input Capture Noise Canceler: disabled
  TCCR1B = 0x1b;

  // TCCR1C value: 0x00  (Timer/Counter 1 Control Register C)
  //
  //
  //
  //
  //
  //
  //   FOC1B          0  Force Output Compare for Channel B
  //   FOC1A          0  Force Output Compare for Channel A
  TCCR1C = 0x00; 
  
  TIMSK1 = 1;
}

uint16_t totalDuration;
uint16_t onDuration;

SIGNAL(TIMER1_OVF_vect)
{
    ICR1 = totalDuration;
    OCR1A = onDuration;
}

void loop()
{
  uint16_t freqMajor = analogRead(A0);
  uint16_t freqMinor = analogRead(A1);
  uint16_t duty = analogRead(A2);
  totalDuration = ((freqMajor << 6) & 0xfc00) | freqMinor;
  onDuration = (((uint32_t)totalDuration) * ((uint32_t)duty)) >> 10;
}
