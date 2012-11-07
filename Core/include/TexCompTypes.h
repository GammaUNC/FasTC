// Copyright 2012 (c) Pavel Krajcevski
// BC7IntTypes.h

// This file contains all of the various platform definitions for fixed width integers
// on various platforms.

// !FIXME! Still needs to be tested on Windows platforms.
#ifndef _TEX_COMP_TYPES_H_
#define _TEX_COMP_TYPES_H_

// Windows?
#ifdef _MSC_VER

typedef __int16 int16;
typedef unsigned __int16 uint16;
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int8 int8;
typedef unsigned __int8 uint8;

typedef unsigned __int64 uint64;
typedef __int64 int64;

#include <tchar.h>
typedef TCHAR CHAR;

// If not, assume GCC, or at least standard defines...
#else 

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef char CHAR;

#endif // _MSC_VER

#endif // _TEX_COMP_TYPES_H_
