#include <alfe/main.h>

class PEWriter
{
public:
    PEWriter() : _outputPosition(0) { }
    void emitHeaders()
    {
        // DOS stub header

        string("MZ");  // DOS exe signature
        word(0x80);    // file size mod 512
        word(1);       // number of 512 byte pages
        word(0);       // relocation items
        word(4);       // size of header in paragraphs
        word(0);       // min paragraphs needed
        word(-1);      // max paragraphs requested
        word(0);       // initial SS
        word(0xb8);    // initial SP
        word(0);       // checksum
        word(0);       // initial IP
        word(0);       // initial CS
        word(0x40);    // file address of relocation table (=code since no relocations)
        word(0);       // overlay number
        dWord(0); dWord(0); // reserved
        word(0);       // OEM ID
        word(0);       // OEM info
        dWord(0); dWord(0); dWord(0); dWord(0); dWord(0); // reserved
        dWord(0x80);   // File address of PE header (00000080)

        // DOS stub

        byte(0xe);     // PUSH CS
        byte(0x1f);    // POP DS
        byte(0xba); word(0xe); // MOV DX,0e
        byte(0xb4); byte(9); // MOV AH,9
        byte(0xcd); byte(0x21); // INT 021
        byte(0xb8); word(0x4c01); // MOV AX,04c01
        byte(0xcd); byte(0x21); // INT 021
        string("This program cannot be run in DOS mode."); byte(0xd); byte(0xd); byte(0xa); byte('$'); byte(0);

        // padding

        word(0); word(0); word(0);

        // PE signature

        string("PE"); byte(0); byte(0);

        // COFF file header

        word(0x14c);   // Machine type i386
        word(1);       // Number of sections
        dWord(0);      // Time/date stamp
        dWord(0);      // File offset of COFF symbol table
        dWord(0);      // Number of entries in symbol table
        word(0xe0);    // Size of optional header
        word(0x30f);   // Characteristics:
          // 1 = relocations stripped
          // 2 = executable
          // 4 = COFF line numbers removed
          // 8 = COFF local symbols removed
          // 0100 = 32 bit
          // 0200 = debugging information removed

        // Optional header

        word(0x10b);   // Magic number (PE32)
        byte(0);       // Linker major version number
        byte(0);       // Linker minor version number
        dWord(0);      // Size of code section

        int peRawSize = _dataEnd - 0x401000;

        dWord(roundUp(peRawSize, 0x200)); // Size of data section
        dWord(0);      // Size of bss section
        dWord(_peEntryPoint); // Address of entry point
        dWord(0x1000); // Base of code
        dWord(0x1000); // Address of data

        // Optional header - windows header

        dWord(0x400000); // Base address
        dWord(0x1000); // Section alignment
        dWord(0x200);  // File alignment
        word(4);       // Major OS version
        word(0);       // Minor OS version
        word(0);       // Major image version
        word(0);       // Minor image version
        word(4);       // Major subsystem version
        word(0);       // Minor subsystem version
        dWord(0);      // reserved
        dWord(roundUp(peRawSize, 0x1000) + 0x1000); // Image size
        dWord(0x200);  // Size of headers
        dWord(0);      // Checksum
        word(3);       // Subsystem: console
        word(0);       // DLL characteristics
        dWord(0x100000); // Size of stack to reserve (1Mb)
        dWord(0x1000); // Size of stack to commit (4Kb)
        dWord(0x100000); // Size of heap to reserve (1Mb)
        dWord(0x1000); // Size of heap to commit (4Kb)
        dWord(0);      // Loader flags (obsolete)
        dWord(0x10);   // Number of data dictionary entries (16)

        // Optional header - data dictionary entries

        dWord(0);      // Export table address
        dWord(0);      // Export table size
        dWord(_codeEnd - 0x400000); // Import table address
        dWord(0x14*(_numDLLs + 1)); // Import table size
        dWord(0);      // Resource table address
        dWord(0);      // Resource table size
        dWord(0);      // Exception table address
        dWord(0);      // Exception table size
        dWord(0);      // Certificate table address
        dWord(0);      // Certificate table size
        dWord(0);      // Base relocation table address
        dWord(0);      // Base relocation table size
        dWord(0);      // Debug data table address
        dWord(0);      // Debug data table size
        dWord(0);      // Architecture-specific data table address
        dWord(0);      // Architecture-specific data table size
        dWord(0);      // Global pointer address
        dWord(0);      // Must be 0
        dWord(0);      // Thread-local storage table address
        dWord(0);      // Thread-local storage table size
        dWord(0);      // Load configuration table address
        dWord(0);      // Load configuration table size
        dWord(0);      // Bound import table address
        dWord(0);      // Bound import table size
        dWord(0x1000); // Import address table address
        dWord(4*(_totalImports + _numDLLs)); // Import address table size
        dWord(0);      // Delay import descriptor address
        dWord(0);      // Delay import descriptor size
        dWord(0);      // COM+ Runtime header address
        dWord(0);      // COM+ Runtime header size
        dWord(0);      // Reserved
        dWord(0);      // Reserved

        // Section table

        string(".unified"); // Section name
        dWord(peRawSize); // Size of section in memory
        dWord(0x1000); // Virtual address of section
        dWord(peRawSize); // Size of raw data
        dWord(0x200);  // File offset of raw data
        dWord(0);      // Pointer to relocations (none)
        dWord(0);      // Pointer to line numbers (none)
        word(0);       // Number of relocations
        word(0);       // Number of line numbers
        dWord(0xe00000e0); // Flags (0e00000e0):
          // 020 - section contains executable code
          // 040 - section contains initialized data
          // 080 - section contains uninitialized data
          // 020000000 - section can be executed as code
          // 040000000 - section can be read
          // 080000000 - section can be written to

        // Padding (0200 - 040 - 040 - 018 - 0e0 - 028 = 060)

        for (int i = 0; i < 0x18; ++i)
            dWord(0);

    }
private:
    AppendableArray<Byte> _output;
    void string(const char* d)
    {
        while (*d != 0) {
            byte(*d);
            ++d;
        }
    }
    void byte(Byte d) { _output.append(d); }
    void word(Word d) { byte(d & 0xff); byte(d >> 8); }
    void dWord(DWord d) { word(d & 0xffff); word(d >> 16); }
    int roundUp(int v, int g) { return (v + g - 1) & -g; }
    int _codeEnd;
    int _dataEnd;
    int _numDLLs;
    int _totalImports;
};

class Program : public ProgramBase
{
    void run()
    {
    }
};