#include "alfe/main.h"

class Program : public ProgramBase
{
    void run()
    {
        const String data = File(_arguments[1], true).contents();
        const Byte* d = data.data();
        String o;
        int lengths[48];
        for (int i = 0; i < 48; ++i) {
            lengths[i] = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
            d += 4;
        }

        int length = lengths[0];
        for (int i = 0; i < 16; ++i) {
            int l = lengths[i];
            if (l != length || l == -1) {
                length = max(length, l);
                if (l == -1)
                    o += String("Warning: missing sniffer item ") + decimal(i) + "\n";
                else
                    o += String("Warning: sniffer packets have inconsistent lengths.\n");
            }
        }
        bool fastSampling = (lengths[16] != -1);
        if (!fastSampling) {
            for (int i = 17; i < 48; ++i)
                if (lengths[i] != -1)
                    o += String("Warning: got some but not all fast-sampling sniffer packets.\n");
        }
        else {
            for (int i = 16; i < 48; ++i)
                if (lengths[i] != length) {
                    length = max(length, lengths[i]);
                    o += String("Warning: sniffer packets have inconsistent lengths.\n");
                }
        }
        if (length == -1) {
            o += String("Warning: sniffer activated but no data received.\n");
            return;
        }

        for (int th = 0; th < length; ++th)
            for (int tl = 0; tl < (fastSampling ? 3 : 1); ++tl) {
                Byte p[16];
                for (int i = 0; i < 16; ++i) {
                    p[i] = d[th + (i + tl*16)*2048];
                    if ((p[i] & 0x80) != 0)
                        o += String("Warning: sniffer packet corrupted.\n");
                    if (i < 8) {
                        if ((p[i] & 0x40) != 0)
                            o += String("Warning: sniffer port C packet corrupted.\n");
                    }
                    else
                        p[i] = ((p[i] & 0x40) >> 6) | ((p[i] & 0x3f) << 2);
                }
                UInt32 cpu =
                      ((p[0x4] & 0x08) != 0 ? 0x80000 : 0)   // 35 A19/S6        O ADDRESS/STATUS: During T1, these are the four most significant address lines for memory operations. During I/O operations, these lines are LOW. During memory and I/O operations, status information is available on these lines during T2, T3, Tw, and T4. S6 is always low.
                    | ((p[0x7] & 0x10) != 0 ? 0x40000 : 0)   // 36 A18/S5        O The status of the interrupt enable flag bit (S5) is updated at the beginning of each clock cycle.
                    | ((p[0x6] & 0x10) != 0 ? 0x20000 : 0)   // 37 A17/S4        O  S4*2+S3 0 = Alternate Data, 1 = Stack, 2 = Code or None, 3 = Data
                    | ((p[0x5] & 0x10) != 0 ? 0x10000 : 0)   // 38 A16/S3        O
                    | ((p[0x4] & 0x10) != 0 ? 0x08000 : 0)   // 39 A15           O ADDRESS BUS: These lines provide address bits 8 through 15 for the entire bus cycle (T1ñT4). These lines do not have to be latched by ALE to remain valid. A15ñA8 are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
                    | ((p[0x2] & 0x10) != 0 ? 0x04000 : 0)   //  2 A14
                    | ((p[0x1] & 0x10) != 0 ? 0x02000 : 0)   //  3 A13
                    | ((p[0x0] & 0x10) != 0 ? 0x01000 : 0)   //  4 A12
                    | ((p[0x3] & 0x08) != 0 ? 0x00800 : 0)   //  5 A11
                    | ((p[0x2] & 0x08) != 0 ? 0x00400 : 0)   //  6 A10
                    | ((p[0x1] & 0x08) != 0 ? 0x00200 : 0)   //  7 A9
                    | ((p[0x0] & 0x08) != 0 ? 0x00100 : 0)   //  8 A8
                    | ((p[0xb] & 0x04) != 0 ? 0x00080 : 0)   //  9 AD7          IO ADDRESS DATA BUS: These lines constitute the time multiplexed memory/IO address (T1) and data (T2, T3, Tw, T4) bus. These lines are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
                    | ((p[0xa] & 0x04) != 0 ? 0x00040 : 0)   // 10 AD6
                    | ((p[0x9] & 0x04) != 0 ? 0x00020 : 0)   // 11 AD5
                    | ((p[0x8] & 0x04) != 0 ? 0x00010 : 0)   // 12 AD4
                    | ((p[0xb] & 0x08) != 0 ? 0x00008 : 0)   // 13 AD3
                    | ((p[0xa] & 0x08) != 0 ? 0x00004 : 0)   // 14 AD2
                    | ((p[0x9] & 0x08) != 0 ? 0x00002 : 0)   // 15 AD1
                    | ((p[0x8] & 0x08) != 0 ? 0x00001 : 0);  // 16 AD0
                UInt8 qs =
                      ((p[0xe] & 0x08) != 0 ? 1 : 0)         // 25 QS0           O QUEUE STATUS: provide status to allow external tracking of the internal 8088 instruction queue. The queue status is valid during the CLK cycle after which the queue operation is performed.
                    | ((p[0xf] & 0x08) != 0 ? 2 : 0);        // 24 QS1           0 = No operation, 1 = First Byte of Opcode from Queue, 2 = Empty the Queue, 3 = Subsequent Byte from Queue
                char qsc[] = ".IES";

                UInt8 s =
                      ((p[0xd] & 0x08) != 0 ? 1 : 0)         // 26 -S0           O STATUS: is active during clock high of T4, T1, and T2, and is returned to the passive state (1,1,1) during T3 or during Tw when READY is HIGH. This status is used by the 8288 bus controller to generate all memory and I/O access control signals. Any change by S2, S1, or S0 during T4 is used to indicate the beginning of a bus cycle, and the return to the passive state in T3 and Tw is used to indicate the end of a bus cycle. These signals float to 3-state OFF during ``hold acknowledge''. During the first clock cycle after RESET becomes active, these signals are active HIGH. After this first clock, they float to 3-state OFF.
                    | ((p[0xc] & 0x08) != 0 ? 2 : 0)         // 27 -S1           0 = Interrupt Acknowledge, 1 = Read I/O Port, 2 = Write I/O Port, 3 = Halt, 4 = Code Access, 5 = Read Memory, 6 = Write Memory, 7 = Passive
                    | ((p[0xf] & 0x04) != 0 ? 4 : 0);        // 28 -S2
                char sc[] = "ARWHCrwp";

                //bool lock      = ((p[0xe] & 0x04) == 0);     // 29 -LOCK    !87  O LOCK: indicates that other system bus masters are not to gain control of the system bus while LOCK is active (LOW). The LOCK signal is activated by the ``LOCK'' prefix instruction and remains active until the completion of the next instruction. This signal is active LOW, and floats to 3-state off in ``hold acknowledge''.
                bool rqgt0     = ((p[0xc] & 0x04) == 0);     // 31 -RQ/-GT0 !87 IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
                //bool rqgt1     = ((p[0xd] & 0x04) == 0);     // 30 -RQ/-GT1     IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
                bool ready     = ((p[0xd] & 0x10) != 0);     // 22 READY        I  READY: is the acknowledgement from the addressed memory or I/O device that it will complete the data transfer. The RDY signal from memory or I/O is synchronized by the 8284 clock generator to form READY. This signal is active HIGH. The 8088 READY input is not synchronized. Correct operation is not guaranteed if the set up and hold times are not met.
                bool test      = ((p[0xc] & 0x10) == 0);     // 23 -TEST        I  TEST: input is examined by the ``wait for test'' instruction. If the TEST input is LOW, execution continues, otherwise the processor waits in an ``idle'' state. This input is synchronized internally during each clock cycle on the leading edge of CLK.
                //bool rd        = ((p[0x7] & 0x08) == 0);     // 32 -RD      !87  O READ: Read strobe indicates that the processor is performing a memory or I/O read cycle, depending on the state of the IO/M pin or S2. This signal is used to read devices which reside on the 8088 local bus. RD is active LOW during T2, T3 and Tw of any read cycle, and is guaranteed to remain HIGH in T2 until the 8088 local bus has floated. This signal floats to 3-state OFF in ``hold acknowledge''.
                //bool intr      = ((p[0xa] & 0x10) != 0);     // 18 INTR     !87 I  INTERRUPT REQUEST: is a level triggered input which is sampled during the last clock cycle of each instruction to determine if the processor should enter into an interrupt acknowledge operation. A subroutine is vectored to via an interrupt vector lookup table located in system memory. It can be internally masked by software resetting the interrupt enable bit. INTR is internally synchronized. This signal is active HIGH.
                bool cpu_clk   = ((p[0x9] & 0x10) != 0);     // 19 CLK          I  CLOCK: provides the basic timing for the processor and bus controller. It is asymmetric with a 33% duty cycle to provide optimized internal timing.
                //bool nmi       = ((p[0xb] & 0x10) != 0);     // 17 NMI      !87 I  NON-MASKABLE INTERRUPT: is an edge triggered input which causes a type 2 interrupt. A subroutine is vectored to via an interrupt vector lookup table located in system memory. NMI is not maskable internally by software. A transition from a LOW to HIGH initiates the interrupt at the end of the current instruction. This input is internally synchronized.

                UInt32 address =
                      ((p[0x4] & 0x02) != 0 ? 0x80000 : 0)   // A12 +A19         O Address bits: These lines are used to address memory and I/O devices within the system. These lines are generated by either the processor or DMA controller.
                    | ((p[0x5] & 0x02) != 0 ? 0x40000 : 0)   // A13 +A18
                    | ((p[0x6] & 0x02) != 0 ? 0x20000 : 0)   // A14 +A17
                    | ((p[0x7] & 0x02) != 0 ? 0x10000 : 0)   // A15 +A16
                    | ((p[0x4] & 0x01) != 0 ? 0x08000 : 0)   // A16 +A15
                    | ((p[0x5] & 0x01) != 0 ? 0x04000 : 0)   // A17 +A14
                    | ((p[0x6] & 0x01) != 0 ? 0x02000 : 0)   // A18 +A13
                    | ((p[0x7] & 0x01) != 0 ? 0x01000 : 0)   // A19 +A12
                    | ((p[0xc] & 0x20) != 0 ? 0x00800 : 0)   // A20 +A11
                    | ((p[0xd] & 0x20) != 0 ? 0x00400 : 0)   // A21 +A10
                    | ((p[0xe] & 0x20) != 0 ? 0x00200 : 0)   // A22 +A9
                    | ((p[0xf] & 0x20) != 0 ? 0x00100 : 0)   // A23 +A8
                    | ((p[0xc] & 0x40) != 0 ? 0x00080 : 0)   // A24 +A7
                    | ((p[0xd] & 0x40) != 0 ? 0x00040 : 0)   // A25 +A6
                    | ((p[0xe] & 0x40) != 0 ? 0x00020 : 0)   // A26 +A5
                    | ((p[0xf] & 0x40) != 0 ? 0x00010 : 0)   // A27 +A4
                    | ((p[0xc] & 0x80) != 0 ? 0x00008 : 0)   // A28 +A3
                    | ((p[0xd] & 0x80) != 0 ? 0x00004 : 0)   // A29 +A2
                    | ((p[0xe] & 0x80) != 0 ? 0x00002 : 0)   // A30 +A1
                    | ((p[0xf] & 0x80) != 0 ? 0x00001 : 0);  // A31 +A0
                UInt8 data =
                      ((p[0x6] & 0x20) != 0 ? 0x80 : 0)      // A2  +D7         IO Data bits: These lines provide data bus bits 0 to 7 for the processor, memory, and I/O devices.
                    | ((p[0x7] & 0x20) != 0 ? 0x40 : 0)      // A3  +D6
                    | ((p[0xc] & 0x01) != 0 ? 0x20 : 0)      // A4  +D5
                    | ((p[0xd] & 0x01) != 0 ? 0x10 : 0)      // A5  +D4
                    | ((p[0xe] & 0x01) != 0 ? 0x08 : 0)      // A6  +D3
                    | ((p[0xf] & 0x01) != 0 ? 0x04 : 0)      // A7  +D2
                    | ((p[0x4] & 0x04) != 0 ? 0x02 : 0)      // A8  +D1
                    | ((p[0x5] & 0x04) != 0 ? 0x01 : 0);     // A9  +D0
                UInt8 dma =
                      ((p[0x0] & 0x01) != 0 ? 0x02 : 0)      // B18 +DRQ1       I  DMA Request: These lines are asynchronous channel requests used by peripheral devices to gain DMA service. They are prioritized with DRQ3 being the lowest and DRQl being the highest. A request is generated by bringing a DRQ line to an active level (high). A DRQ line must be held high until the corresponding DACK line goes active.
                    | ((p[0x8] & 0x01) != 0 ? 0x04 : 0)      // B6  +DRQ2
                    | ((p[0x2] & 0x01) != 0 ? 0x08 : 0)      // B16 +DRQ3
                    | ((p[0xb] & 0x20) == 0 ? 0x10 : 0)      // B19 -DACK0       O -DMA Acknowledge: These lines are used to acknowledge DMA requests (DRQ1-DRQ3) and to refresh system dynamic memory (DACK0). They are active low.
                    | ((p[0x1] & 0x01) == 0 ? 0x20 : 0)      // B17 -DACK1
                    | ((p[0x8] & 0x40) == 0 ? 0x40 : 0)      // B26 -DACK2
                    | ((p[0x3] & 0x01) == 0 ? 0x80 : 0);     // B15 -DACK3
                UInt8 irq =
                      ((p[0xa] & 0x01) != 0 ? 0x04 : 0)      // B4  +IRQ2       I  Interrupt Request lines: These lines are used to signal the processor that an I/O device requires attention. An Interrupt Request is generated by raising an IRQ line (low to high) and holding it high until it is acknowledged by the processor (interrupt service routine).
                    | ((p[0x9] & 0x40) != 0 ? 0x08 : 0)      // B25 +IRQ3
                    | ((p[0xa] & 0x40) != 0 ? 0x10 : 0)      // B24 +IRQ4
                    | ((p[0xb] & 0x40) != 0 ? 0x20 : 0)      // B23 +IRQ5
                    | ((p[0x8] & 0x20) != 0 ? 0x40 : 0)      // B22 +IRQ6
                    | ((p[0x9] & 0x20) != 0 ? 0x80 : 0);     // B21 +IRQ7

                bool ior       = ((p[0x0] & 0x02) == 0);     // B14 -IOR         O -I/O Read Command: This command line instructs an I/O device to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool iow       = ((p[0x1] & 0x02) == 0);     // B13 -IOW         O -I/O Write Command: This command line instructs an I/O device to read the data on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool memr      = ((p[0x2] & 0x02) == 0);     // B12 -MEMR        O Memory Read Command: This command line instructs the memory to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
                bool memw      = ((p[0x3] & 0x02) == 0);     // B11 -MEMW        O Memory Write Command: This command line instructs the memory to store the data present on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.

                //bool bus_reset = ((p[0x0] & 0x20) != 0);     // B2  +RESET DRV   O This line is used to reset or initialize system logic upon power-up or during a low line voltage outage. This signal is synchronized to the falling edge of clock and is active high.
                //bool iochchk   = ((p[0x5] & 0x20) == 0);     // A1  -I/O CH CK  I  -I/O Channel Check: This line provides the processor with parity (error) information on memory or devices in the I/O channel. When this signal is active low, a parity error is indicated.
                bool iochrdy   = ((p[0x6] & 0x04) != 0);     // A10 +I/O CH RDY I  I/O Channel Ready: This line, normally high (ready), is pulled low (not ready) by a memory or I/O device to lengthen I/O or memory cycles. It allows slower devices to attach to the I/O channel with a minimum of difficulty. Any slow device using this line should drive it low immediately upon detecting a valid address and a read or write command. This line should never be held low longer than 10 clock cycles. Machine cycles (I/O or memory) are extended by an integral number of CLK cycles (210 ns).
                bool aen       = ((p[0x7] & 0x04) != 0);     // A11 +AEN         O Address Enable: This line is used to de-gate the processor and other devices from the I/O channel to allow DMA transfers to take place. When this line is active (high), the DMA controller has control of the address bus, data bus, read command lines (memory and I/O), and the write command lines (memory and I/O).
                //bool bus_clk   = ((p[0xa] & 0x20) != 0);     // B20 CLOCK        O System clock: It is a divide-by-three of the oscillator and has a period of 210 ns (4.77 MHz). The clock has a 33% duty cycle.
                bool bus_ale   = ((p[0xa] & 0x80) != 0);     // B28 +ALE         O Address Latch Enable: This line is provided by the 8288 Bus Controller and is used on the system board to latch valid addresses from the processor. It is available to the I/O channel as an indicator of a valid processor address (when used with AEN). Processor addresses are latched with the failing edge of ALE.
                bool tc        = ((p[0xb] & 0x80) != 0);     // B27 +T/C         O Terminal Count: This line provides a pulse when the terminal count for any DMA channel is reached. This signal is active high.

                //UInt16 jumpers =  // Should always be 0
                //      ((p[0x0] & 0x04) != 0 ? 0x00001 : 0)    // JP4/4
                //    | ((p[0x1] & 0x04) != 0 ? 0x00002 : 0)    // JP4/3
                //    | ((p[0x1] & 0x20) != 0 ? 0x00004 : 0)    // JP9/4
                //    | ((p[0x2] & 0x04) != 0 ? 0x00008 : 0)    // JP4/2
                //    | ((p[0x2] & 0x20) != 0 ? 0x00010 : 0)    // JP9/3
                //    | ((p[0x3] & 0x04) != 0 ? 0x00020 : 0)    // JP4/1
                //    | ((p[0x3] & 0x10) != 0 ? 0x00040 : 0)    // JP5/1
                //    | ((p[0x3] & 0x20) != 0 ? 0x00080 : 0)    // JP9/2
                //    | ((p[0x4] & 0x20) != 0 ? 0x00100 : 0)    // JP9/1
                //    | ((p[0x8] & 0x80) != 0 ? 0x00200 : 0)    // JP7/2
                //    | ((p[0x9] & 0x01) != 0 ? 0x00400 : 0)    // JP3/1
                //    | ((p[0x9] & 0x80) != 0 ? 0x00800 : 0)    // JP7/1
                //    | ((p[0xf] & 0x10) != 0 ? 0x01000 : 0)    // JP6/1
                //    | ((p[0x6] & 0x08) == 0 ? 0x02000 : 0)    // 33 MN/-MX     I    MINIMUM/MAXIMUM: indicates what mode the processor is to operate in. The two modes are discussed in the following sections. (zero for maximum mode on PC/XT)
                //    | ((p[0x5] & 0x08) == 0 ? 0x04000 : 0)    // 34 -SS0        O   Pin 34 is always high in the maximum mode.
                //    | ((p[0xb] & 0x01) == 0 ? 0x08000 : 0)    // Serial - should always be 1
                //    | ((p[0x8] & 0x10) != 0 ? 0x10000 : 0)    // GND - should always be 0
                //    | ((p[0xe] & 0x10) != 0 ? 0x20000 : 0);   // RESET - should always be 0

                o += String(hex(cpu, 5, false)) + " " +
                    codePoint(qsc[qs]) + codePoint(sc[s]) +
                    (rqgt0 ? "G" : ".") + (ready ? "." : "z") +
                    (test ? "T" : ".") + // (cpu_clk ? "C" : ".") + "  " +
                    hex(address, 5, false) + " " + hex(data, 2, false) + " " +
                    hex(dma, 2, false) + " " + hex(irq, 2, false) + " " +
                    (ior ? "R" : ".") + (iow ? "W" : ".") +
                    (memr ? "r" : ".") + (memw ? "w" : ".") +
                    (iochrdy ? "." : "z") + (aen ? "D" : ".") +
                    //(bus_ale ? "a" : ".") +
                    (tc ? "T" : ".") + "\n";
            }
        File(_arguments[2], true).save(o);
    }
};