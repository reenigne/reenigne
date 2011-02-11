#ifndef INCLUDED_INTEGER_TYPES_H
#define INCLUDED_INTEGER_TYPES_H

typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned int       UInt32;
typedef signed int         SInt32;

typedef void               Void;

#if sizeof(Void*)==sizeof(UInt32)
typedef SInt32             PSInt;
typedef UInt32             PUInt;
typedef PSInt              PointerDifference;
#endif

#endif // INCLUDED_INTEGER_TYPES_H
