#include "unity/main.h"

int inputValues(int inputs, int connection)
{
    switch (connection) {
        case 0: return 0;
        case 1: return 3;
        case 2: return ((inputs & 2) != 0) ? 3 : 0;
        case 3: return ((inputs & 1) != 0) ? 3 : 0;
        case 4: return 3; //2;
    }
}

class Program : public ProgramBase
{
public:
    void run()
    {
        for (int gate = 0; gate < 0x100; ++gate) {
            bool found[0x10];
            int circuits[0x10];
            for (int i = 0; i < 0x10; ++i)
                found[i] = false;
            int count = 0;
            for (int circuit = 0; circuit < 5*5*5; ++circuit) {
                int aConnection = circuit / 25;
                int bConnection = (circuit / 5) % 5;
                int cConnection = circuit % 5;
                int outputs = 0;
                int inputs;
                for (inputs = 0; inputs < 4; ++inputs) {
                    int aValues = inputValues(inputs, aConnection);
                    int bValues = inputValues(inputs, bConnection);
                    int cValues = inputValues(inputs, cConnection);
                    int inputs0 = ((aValues & 1) << 2) | ((bValues & 1) << 1) | (cValues & 1);
                    int inputs1 = ((aValues & 2) << 1) | (bValues & 2) | ((cValues & 2) >> 1);
                    bool y0 = (((gate >> inputs0) & 1) != 0);
                    bool y1 = (((gate >> inputs1) & 1) != 0);
                    int y = (y0 ? 1 : 0) | (y1 ? 2 : 0);
                    // Cases: 
                    //   y = 0: Output converges to 0
                    //   y = 1: Output oscillates
                    //   y = 2: Output is bistable
                    //   y = 3: Output converges to 1
                    if (y == 1 || y == 2)
                        break;
                    if (y == 3)
                        outputs |= (1 << inputs);
                }
                if (inputs < 4)
                    continue;
                if (!found[outputs]) {
                    found[outputs] = true;
                    circuits[outputs] = circuit;
                    ++count;
                }
            }
            String s = hex(gate, 2) + ": ";
            for (int i = 0; i < 8; ++i)
                if (((gate >> i) & 1) != 0)
                    s += "*";
                else
                    s += ".";
            s += " : " + String(decimal(count)).alignRight(2) + " ";
            for (int i = 0; i < 0x10; ++i)
                if (found[i])
                    s += "*";
                else
                    s += ".";
            s += " : ";
            for (int i = 0; i < 0x10; ++i) {
                if (found[i]) {
                    int circuit = circuits[i];
                    s += codePoint("01DEY"[circuit / 25]);
                    s += codePoint("01DEY"[(circuit / 5) % 5]);
                    s += codePoint("01DEY"[circuit % 5]);
                }
                else
                    s += "---";
                s += " ";
            }
            _console.write(s + "\n");
        }
    }
};