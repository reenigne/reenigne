#include "alfe/main.h"
#include "zlib/zlib.h"

#ifndef INCLUDED_RDIF_H
#define INCLUDED_RDIF_H

class DiskImage
{
public:
    DiskImage() : _version(0) { }
    ~DiskImage() { _blocks.release(); }
    void clear()
    {
        _blocks.release();
        _isRaw = false;
        _version = 0;
        _compressed = false;
        _creator = "";
        _label = "";
        _description = "";
        _medium = miniFloppy;
        _tracksPerInch = 48;
        _writeEnable = true;
        _rotationsPerMinute = 300;
        _bitsPerSecond = 250000;
        _encoding = modifiedFrequencyModulation;
        _heads = 2;
        _defaultBytesPerSector = 512;
        _defaultSectorsPerTrack = 9;
        _blockCount = 0;
    }
    void load(String data)
    {
        _blocks.release();
        int l = data.length();
        create(l);
        int magic = readInt(data, 0);
        if (magic != 0x46494452)
            _isRaw = true;
        if (_isRaw) {
            Block* block = new Block;
            _blocks.add(block);
            block->_size = l;
            block->_cylinder = 0;
            block->_head = 0;
            block->_position = 0;
            block->_dataRate = _bitsPerSecond * 60 / _rotationsPerMinute;
            block->_type = Block::justData;
            block->_trackWidth = 0x100;
            block->_initialFlux = 0x40;
            block->_compressed = false;
            block->_data.allocate(l);
            for (int i = 0; i < l; ++i)
                block->_data[i] = data[i];
            return;
        }
        _version = readInt(data, 4);
        if (_version != 0)
            throw Exception("Unknown RDIF version");
        _compressed = (readInt(data, 8) != 0);
        _creator = data.subString(readInt(data, 12), readInt(data, 16));
        _label = data.subString(readInt(data, 20), readInt(data, 24));
        _description = data.subString(readInt(data, 28), readInt(data, 32));
        switch (readInt(data, 36)) {
            case 0: _medium = eightInch; break;
            case 1: _medium = miniFloppy; break;
            case 2: _medium = microFloppy; break;
            default:
                throw Exception("Unknown media type");
        }
        _tracksPerInch = readInt(data, 40);
        _writeEnable = (readInt(data, 44) != 0);
        _rotationsPerMinute = readInt(data, 48);
        _bitsPerSecond = readInt(data, 52);
        switch (readInt(data, 56)) {
            case 0: _encoding = frequencyModulation; break;
            case 1: _encoding = modifiedFrequencyModulation; break;
            default:
                throw Exception("Unknown encoding type");
        }
        _heads = readInt(data, 60);
        _defaultBytesPerSector = readInt(data, 64);
        _defaultSectorsPerTrack = readInt(data, 68);
        _blockCount = readInt(data, 72);
        for (int i = 0; i < _blockCount; ++i) {
            Block* block = new Block(data, i);
            _blocks.add(block);
        }
    }
    String save()
    {
        // TODO
    }
    void create(int length)
    {
        clear();
        switch (length) {
            case 160*1024:
                _isRaw = true;
                _defaultSectorsPerTrack = 8;
                break;
            case 180*1024:
                _isRaw = true;
                break;
            case 320*1024:
                _isRaw = true;
                _defaultSectorsPerTrack = 8;
                break;
            case 360*1024:
                _isRaw = true;
                break;
            case 720*1024:
                _isRaw = true;
                _medium = microFloppy;
                break;
            case 1200*1024:
                _isRaw = true;
                _defaultSectorsPerTrack = 15;
                _tracksPerInch = 96;
                _rotationsPerMinute = 360;
                _bitsPerSecond = 500000;
                break;
            case 1440*1024:
                _isRaw = true;
                _defaultSectorsPerTrack = 18;
                _medium = microFloppy;
                _bitsPerSecond = 500000;
                break;
            case 2880*1024:
                _isRaw = true;
                _defaultSectorsPerTrack = 36;
                _medium = microFloppy;
                _bitsPerSecond = 1000000;
                break;
            default:
                _isRaw = false;
        }
        if (_medium == microFloppy)
            _tracksPerInch = 135;
        if (_isRaw)
            _compressed = false;
    }

    // For XT Server.
    void bios(Array<Byte>* data, Byte* hostBytes)
    {
        if (hostBytes[19] != 0)
            return;
        Byte sectorCount = hostBytes[1];
        Byte operation = hostBytes[2];
        Byte sector = hostBytes[3] & 0x3f;
        Word track = hostBytes[4] | ((hostBytes[3] & 0xc0) << 2);
        Byte head = hostBytes[6];
        int sectorSize = 128 << hostBytes[10];
        Byte sectorsPerTrack = hostBytes[11];
        Byte gapLength = hostBytes[12];
        Byte dataLength = hostBytes[13];
        Byte formatGapLength = hostBytes[14];
        Byte formatFillByte = hostBytes[15];
//        Block* block;
        switch (hostBytes[2]) {
            case 2:
                // Read
                //block = findBlock(track << 8, head);
                //if (block == 0) {
                //    hostBytes[19] = 4;
                //    return;
                //}

                //data->allocate(sectorSize * sectorCount);
                //do {

                //} while (true);

                break;
            case 3:
                // Write
                break;
            case 4:
                // Verify
                break;
            case 5:
                // Format
                break;
            default:
                hostBytes[19] = 1;
                return;
        }
    }

private:
    static int readInt(String data, int p)
    {
        return static_cast<int>(readSInt32(data, p));
    }
    static SInt32 readSInt32(String data, int p)
    {
        return static_cast<SInt32>(readUInt32(data, p));
    }
    static UInt32 readUInt32(String data, int p)
    {
        return readUInt16(data, p) | (readUInt16(data, p + 2) << 16);
    }
    static UInt16 readUInt16(String data, int p)
    {
        return readUInt8(data, p) | (readUInt8(data, p + 1) << 8);
    }
    static UInt16 readUInt8(String data, int p)
    {
        if (p >= data.length())
            return 0;
        return data[p];
    }
    //void findBlock(SInt32 track, int head)
    //{
    //    Block* b = _blocks.getNext();
    //    Block* p = b;
    //    while (b != 0) {
    //        SInt32 start = b->_cylinder - (b->_trackWidth / 2);
    //        if (track >= start && track < start + b->_trackWidth && _he)

    //    }
    //}
    class Block : public LinkedListMember<Block>
    {
    public:
        Block() { }
        Block(String data, int i)
        {
            int p = i*36 + 76;
            _size = readInt(data, p + 4);
            _data.allocate(_size);
            int offset = readInt(data, p);
            for (int j = 0; j < _size; ++j)
                _data[j] = data[offset + j];
            _cylinder = readSInt32(data, p + 8);
            _head = readInt(data, p + 12);
            _position = readSInt32(data, p + 16);
            _dataRate = readUInt32(data, p + 20);
            switch (readInt(data, p + 24)) {
                case 0: _type = justData; break;
                case 1: _type = fluxReversals; break;
                case 2: _type = rawFlux; break;
                default:
                    throw Exception("Unknown RDIF data type");
            }
            _trackWidth = readSInt32(data, p + 28);
            _initialFlux = readSInt32(data, p + 32);
            _compressed = (readSInt32(data, p + 36) != 0);
        }

        int _size;
        SInt32 _cylinder;
        int _head;
        SInt32 _position;
        UInt32 _dataRate;
        enum Type { justData, fluxReversals, rawFlux } _type;
        SInt32 _trackWidth;
        SInt32 _initialFlux;
        bool _compressed;
        Array<Byte> _data;
    };

    bool _isRaw;
    int _version;
    bool _compressed;
    String _creator;
    String _label;
    String _description;
    enum Medium { eightInch, miniFloppy, microFloppy } _medium;
    int _tracksPerInch;
    bool _writeEnable;
    int _rotationsPerMinute;  // Nominal (0 = variable; CLV or zone bit recording)
    int _bitsPerSecond;
    enum Encoding { frequencyModulation, modifiedFrequencyModulation } _encoding;
    int _heads;
    int _defaultBytesPerSector;
    int _defaultSectorsPerTrack;
    LinkedList<Block> _blocks;
    int _blockCount;
};

#endif // INCLUDED_RDIF_H
