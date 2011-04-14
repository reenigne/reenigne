#ifndef INCLUDED_INTEGER_TYPES_H
#define INCLUDED_INTEGER_TYPES_H

typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned int       UInt32;
typedef signed int         SInt32;
#ifdef _WIN32
typedef unsigned _int64    UInt64;
typedef signed _int64      SInt64;
#else
#include <inttypes.h>
typedef uint64_t           UInt64;
typedef int64_t            SInt64;
#endif

typedef UInt8              Byte;

typedef void               Void;

typedef SInt32             PSInt;
typedef UInt32             PUInt;
typedef PSInt              PointerDifference;

#endif // INCLUDED_INTEGER_TYPES_H
