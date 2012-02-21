#include "alfe/main.h"

#ifndef INCLUDED_INTEGER_TYPES_H
#define INCLUDED_INTEGER_TYPES_H

typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef char               Int8;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef short int          Int16;
typedef unsigned int       UInt32;
typedef signed int         SInt32;
typedef int                Int32;
#ifdef _WIN32
typedef unsigned _int64    UInt64;
typedef signed _int64      SInt64;
typedef _int64             Int64;
#else
#include <inttypes.h>
typedef uint64_t           UInt64;
typedef int64_t            SInt64;
typedef int64_t            Int64;
#endif

typedef UInt8              Byte;
typedef UInt16             Word;
typedef UInt32             DWord;
typedef UInt64             QWord;

typedef void               Void;

typedef SInt32             PSInt;
typedef UInt32             PUInt;
typedef PSInt              PointerDifference;

#endif // INCLUDED_INTEGER_TYPES_H
