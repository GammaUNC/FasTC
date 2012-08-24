
// Copyright 2012 (c) Pavel Krajcevski
// BC7IntTypes.h

// This file contains all of the various platform definitions for fixed width integers
// on various platforms.

// !FIXME! Still needs to be tested on Windows platforms.


#ifdef _MSC_VER
typedef __int16 int16;
typedef __uint16 uint16;
typedef __int32 int32;
typedef __uint32 uint32;
typedef __int8 int8;
typedef __uint8 uint8;

#else

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

#endif
