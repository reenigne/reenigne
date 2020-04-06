#ifndef INCLUDED_SHA256_H
#define INCLUDED_SHA256_H

#include "alfe/integer_types.h"

// Based on Brad Conte's implementation at
// https://github.com/B-Con/crypto-algorithms

class SHA256Hash
{
    Byte _data[32];
public:
    bool bit(int i) { return (_data[i >> 3] & (1 << (i & 7))) != 0; }
    class Hasher
    {
    public:
        Hasher()
        {
            _datalen = 0;
            _bitlen = 0;
            _state[0] = 0x6a09e667;
            _state[1] = 0xbb67ae85;
            _state[2] = 0x3c6ef372;
            _state[3] = 0xa54ff53a;
            _state[4] = 0x510e527f;
            _state[5] = 0x9b05688c;
            _state[6] = 0x1f83d9ab;
            _state[7] = 0x5be0cd19;
        }
        void update(const Byte* data, size_t len)
        {
            DWord i;

            for (i = 0; i < len; ++i) {
                _data[_datalen] = data[i];
                _datalen++;
                if (_datalen == 64) {
                    transform();
                    _bitlen += 512;
                    _datalen = 0;
                }
            }
        }
        void final(Byte* hash)
        {
            int i = _datalen;

            // Pad whatever data is left in the buffer.
            if (_datalen < 56) {
                _data[i++] = 0x80;
                while (i < 56)
                    _data[i++] = 0x00;
            }
            else {
                _data[i++] = 0x80;
                while (i < 64)
                    _data[i++] = 0x00;
                transform();
                memset(_data, 0, 56);
            }

            // Append to the padding the total message's length in bits and transform.
            _bitlen += _datalen * 8;
            _data[63] = _bitlen & 0xff;
            _data[62] = (_bitlen >> 8) & 0xff;
            _data[61] = (_bitlen >> 16) & 0xff;
            _data[60] = (_bitlen >> 24) & 0xff;
            _data[59] = (_bitlen >> 32) & 0xff;
            _data[58] = (_bitlen >> 40) & 0xff;
            _data[57] = (_bitlen >> 48) & 0xff;
            _data[56] = (_bitlen >> 56) & 0xff;
            transform();

            // Since this implementation uses little endian byte ordering and SHA uses big endian,
            // reverse all the bytes when copying the final state to the output hash.
            for (i = 0; i < 4; ++i) {
                hash[i] = (_state[0] >> (24 - i * 8)) & 0xff;
                hash[i + 4] = (_state[1] >> (24 - i * 8)) & 0xff;
                hash[i + 8] = (_state[2] >> (24 - i * 8)) & 0xff;
                hash[i + 12] = (_state[3] >> (24 - i * 8)) & 0xff;
                hash[i + 16] = (_state[4] >> (24 - i * 8)) & 0xff;
                hash[i + 20] = (_state[5] >> (24 - i * 8)) & 0xff;
                hash[i + 24] = (_state[6] >> (24 - i * 8)) & 0xff;
                hash[i + 28] = (_state[7] >> (24 - i * 8)) & 0xff;
            }
        }
    private:
        // Right rotation
        DWord rot(DWord a, int b) { return (a >> b) | (a << (32 - b)); }
        DWord sig0(DWord x) { return rot(x, 7) ^ rot(x, 18) ^ (x >> 3); }
        DWord sig1(DWord x) { return rot(x, 17) ^ rot(x, 19) ^ (x >> 10); }
        void transform()
        {
            static const DWord k[64] = {
                0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
            };

            DWord m[64];
            int i;
            int j;

            for (i = 0, j = 0; i < 16; ++i, j += 4)
                m[i] = (_data[j] << 24) | (_data[j + 1] << 16) | (_data[j + 2] << 8) | (_data[j + 3]);
            for (; i < 64; ++i)
                m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];

            DWord a = _state[0];
            DWord b = _state[1];
            DWord c = _state[2];
            DWord d = _state[3];
            DWord e = _state[4];
            DWord f = _state[5];
            DWord g = _state[6];
            DWord h = _state[7];

            for (i = 0; i < 64; ++i) {
                DWord t1 = h + (rot(e, 6) ^ rot(e, 11) ^ rot(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + m[i];
                DWord t2 = (rot(a, 2) ^ rot(a, 13) ^ rot(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
            }

            _state[0] += a;
            _state[1] += b;
            _state[2] += c;
            _state[3] += d;
            _state[4] += e;
            _state[5] += f;
            _state[6] += g;
            _state[7] += h;
        }

        Byte _data[64];
        DWord _datalen;
        unsigned long long _bitlen;
        DWord _state[8];
    };

    SHA256Hash(const Byte* data, int length)
    {
        Hasher h;
        h.update(data, length);
        h.final(_data);
    }
    SHA256Hash(File file)
    {
        Hasher h;
        FileStream stream = file.tryOpenRead();
        if (!stream.valid()) {
            // We don't want to throw an exception in this situation, since for
            // find_duplicates we expect this will happen frequently, for files
            // we don't have read access to. Hash the path instead to avoid
            // extraneous duplicates.
            String p = file.path();
            h.update(&p[0], p.length());
        }
        else {
            UInt64 size = stream.size();
            DWORD crc = 0xffffffff;
            static const int bufferSize = 0x10000;
            Array<UInt8> buffer(bufferSize);
            while (size > 0) {
                int s =
                    static_cast<int>(min(static_cast<UInt64>(bufferSize), size));
                stream.read(&buffer[0], s);
                h.update(&buffer[0], s);
                size -= s;
            }
        }
        h.final(_data);
    }
    SHA256Hash(const SHA256Hash& other)
    {
        for (int i = 0; i < 32; ++i)
            _data[i] = other._data[i];
    }
    SHA256Hash(Hasher& hasher)
    {
        hasher.final(_data);
    }
    bool operator==(const SHA256Hash& other)
    {
        for (int i = 0; i < 32; ++i)
            if (_data[i] != other._data[i])
                return false;
        return true;
    }
    String toString()
    {
        static const char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        char s[65];
        for (int i = 0; i < 32; ++i) {
            s[i*2] = hexDigits[_data[i] >> 4];
            s[i*2 + 1] = hexDigits[_data[i] & 0x0f];
        }
        s[64] = 0;
        // This is a bit strange - the String constructor taking a char* and a
        // size doesn't copy the data.
        return String(s, String());
    }
    const Byte* data() { return &_data[0]; }
};

#endif // INCLUDED_SHA256_H
