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
Word currentCount;
#ifdef SCHEME_Y
Word phase;
Word interruptCount;
Word lastCount;
#endif
#ifdef SCHEME_X
bool nextDoOldInterrupt;
bool currentDoOldInterrupt;
#endif
struct SongRecord
{
    Word _countA;
    Word _countB;
    Word _countC;
    Word _countD;
    Byte _pulseWidthA;
    Byte _pulseWidthB;
    Byte _pulseWidthC;
    Byte _pulseWidthD;
};
SongRecord* songStart;
SongRecord* songPointer;
SongRecord* songEnd;


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
    lastCount = currentCount;
#endif
#ifdef SCHEME_X
    if (currentDoOldInterrupt)
        doOldInterrupt();
    currentDoOldInterrupt = nextDoOldInterrupt;
#endif
    currentCount = nextCount;
    outportb(0x40, nextCount & 0xff);
    outportb(0x40, nextCount >> 8);
}

void doOldInterrupt()
{
    if (songPointer == songEnd)
        songPointer = songStart;
#ifndef SCHEME_X
    pulseWidthA = songPointer->_pulseWidthA;
    countA = songPointer->_countA;
#endif
    pulseWidthB = songPointer->_pulseWidthB;
    pulseWidthC = songPointer->_pulseWidthC;
    pulseWidthD = songPointer->_pulseWidthD;
    countB = songPointer->_countB;
    countC = songPointer->_countC;
    countD = songPointer->_countD;
    ++songPointer;
    if (pulseWidthD != 0) {
        setInterrupt(8, interrupt8_4);
        return;
    }
    if (pulseWidthC != 0) {
        setInterrupt(8, interrupt8_3);
        return;
    }
    if (pulseWidthB != 0) {
        setInterrupt(8, interrupt8_2);
        return;
    }
    setInterrupt(8, interrupt8_1);
}


// 1 channel

void interrupt8_1()  // Just finished counting down from lastCount, now counting down from currentCount
{
    interruptStart();
    nextCount = countA;
    pulseWidthNext = pulseWidthA;
#if SCHEME_X
    nextDoOldInterrupt = true;
#endif
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
#if SCHEME_X
        nextDoOldInterrupt = true;
#else
        pulseWidthNext += pulseWidthA;
#endif
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
#if SCHEME_X
    nextDoOldInterrupt = true;
#endif
    coalesceB();
    goto done;
nextB:
    nextCount = phaseB;
    phaseB = countB;
    pulseWidthNext = pulseWidthB;
#if SCHEME_X
    nextDoOldInterrupt = false;
#endif
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
#if SCHEME_X
    nextDoOldInterrupt = true;
#endif
    coalesceB();
    coalesceC();
    goto done;
nextBC:
    if (phaseC < phaseB)
        goto nextC;
    nextCount = phaseB;
    phaseB = countB;
    pulseWidthNext = pulseWidthB;
#if SCHEME_X
    nextDoOldInterrupt = false;
#endif
    coalesceA();
    coalesceC();
    goto done;
nextC:
    nextCount = phaseC;
    phaseC = countC;
    pulseWidthNext = pulseWidthC;
#if SCHEME_X
    nextDoOldInterrupt = false;
#endif
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
#if SCHEME_X
    nextDoOldInterrupt = true;
#endif
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
#if SCHEME_X
    nextDoOldInterrupt = false;
#endif
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
#if SCHEME_X
    nextDoOldInterrupt = false;
#endif
    coalesceA();
    coalesceB();
    coalesceD();
    goto done;
nextD:
    nextCount = phaseD;
    phaseD = countD;
    pulseWidthNext = pulseWidthD;
#if SCHEME_X
    nextDoOldInterrupt = false;
#endif
    coalesceA();
    coalesceB();
    coalesceC();
done:
    interruptEnd();
}


// We need the count and pulseWidth values once for each channel, but they're
// not changed within the interrupt proper. Just update all copies when the
// frequency or volume changes?
