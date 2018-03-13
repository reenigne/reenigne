// Scheme X: the outer interrupt is triggered by audio channel A pulse (more accurate)
// Scheme Y: the outer interrupt is triggered whichever audio channel is pulsing when the time runs out (faster)
// Implement both schemes: scheme Y will suffice for note/volume changes, scheme X will probably be necessary for ISAV
//   ISAV currently has a slop of about 2 scanlines (need to set vertical active to/from 1 row) but might be able to relax this a bit
//   If using ISAV with speed-critical code, might want to store all the flags in a bitfield in a register


// 1 channel

Byte pulseWidth;
Byte pulseWidthNext;
static const Word coalesceCount;
Word phaseA;
Word countA;
Byte pulseWidthA;
Word nextCount;
Word lastCount;
Word currentCount;
Word phase;
Word interruptCount;

void interruptStart()
{
#ifdef SCHEME_X
    if (pulseWidth != 0)
#endif
        outportb(0x42, pulseWidth);
    pulseWidth = pulseWidthNext;
}

void interruptEnd()
{
#ifdef SCHEME_Y
    phase -= lastCount;
    if (phase < 0) {
        phase += interruptCount;
        doOldInterrupt();
    }
#endif
    lastCount = currentCount;
    currentCount = nextCount;
    outportb(0x40, nextCount & 0xff);
    outportb(0x40, nextCount >> 8);
}

void interrupt8_1()  // Just finished counting down from lastCount, now counting down from currentCount
{
    interruptStart();
    Word nextCount = countA;
    pulseWidthNext = pulseWidthA;
    interruptEnd();
}


// 2 channels

Word phaseB;
Word countB;
Byte pulseWidthB;

void coalesceA()
{
    phaseA -= nextCount;
    if (phaseA < coalesceCount) {
        phaseA += countA;
        pulseWidthNext += pulseWidthA;
    }
}

void coalesceB()
{
    phaseB -= nextCount;
    if (phaseB < coalesceCount) {
        phaseB += countB;
        pulseWidthNext += pulseWidthB;
    }
}

void interrupt8_2()
{
    interruptStart();
    if (phaseB < phaseA)
        goto nextB;
    nextCount = phaseA;
    phaseA = countA;
    pulseWidthNext = pulseWidthA;
    coalesceB();
    goto done;
nextB:
    nextCount = phaseB;
    phaseB = countB;
    pulseWidthNext = pulseWidthB;
    coalesceA();
done:
    interruptEnd();
}


// 3 channels

Word phaseC;
Word countC;
Byte pulseWidthC;

void coalesceC()
{
    phaseC -= nextCount;
    if (phaseC < coalesceCount) {
        phaseC += countC;
        pulseWidthNext += pulseWidthC;
    }
}

void interrupt8_3()
{
    interruptStart();
    if (phaseB < phaseA)
        goto nextBC;
    if (phaseC < phaseA)
        goto nextC;
    nextCount = phaseA;
    phaseA = countA;
    pulseWidthNext = pulseWidthA;
    coalesceB();
    coalesceC();
    goto done;
nextBC:
    if (phaseC < phaseB)
        goto nextC;
    nextCount = phaseB;
    phaseB = countB;
    pulseWidthNext = pulseWidthB;
    coalesceA();
    coalesceC();
    goto done;
nextC:
    nextCount = phaseC;
    phaseC = countC;
    pulseWidthNext = pulseWidthC;
    coalesceA();
    coalesceB();
done:
    interruptEnd();
}


// 4 channels

Word phaseD;
Word countD;
Byte pulseWidthD;

void coalesceD()
{
    phaseD -= nextCount;
    if (phaseD < coalesceCount) {
        phaseD += countD;
        pulseWidthNext += pulseWidthD;
    }
}

void interrupt8_4()
{
    interruptStart();
    if (phaseB < phaseA)
        goto nextBCD;
    if (phaseC < phaseA)
        goto nextCD;
    if (phaseD < phaseA)
        goto nextD;
    nextCount = phaseA;
    phaseA = countA;
    pulseWidthNext = pulseWidthA;
    coalesceB();
    coalesceC();
    coalesceD();
    goto done;
nextBCD:
    if (phaseC < phaseB)
        goto nextCD;
    if (phaseD < phaseB)
        goto nextD;
    nextCount = phaseB;
    phaseB = countB;
    pulseWidthNext = pulseWidthB;
    coalesceA();
    coalesceC();
    coalesceD();
    goto done;
nextCD:
    if (phaseD < phaseC)
        goto nextD;
    nextCount = phaseC;
    phaseC = countC;
    pulseWidthNext = pulseWidthC;
    coalesceA();
    coalesceB();
    coalesceD();
    goto done;
nextD:
    nextCount = phaseD;
    phaseD = countD;
    pulseWidthNext = pulseWidthD;
    coalesceA();
    coalesceB();
    coalesceC();
done:
    interruptEnd(nextCount);
}


// We need the count and pulseWidth values once for each channel, but they're
// not changed within the interrupt proper. Just update all copies when the
// frequency or volume changes?
