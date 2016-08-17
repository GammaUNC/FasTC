// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

// This file contains all of the various platform definitions for fixed width integers
// on various platforms.

// !FIXME! Still needs to be tested on Windows platforms.
#ifndef _TEX_COMP_TYPES_H_
#define _TEX_COMP_TYPES_H_

#include "FasTC/BaseConfig.h"

// Do we support C++11?
#ifdef FASTC_BASE_HAS_CPP11_TYPES
#include <cstdint>

typedef int8_t int8;
typedef uint8_t uint8;

typedef int16_t int16;
typedef uint16_t uint16;

typedef int32_t int32;
typedef uint32_t uint32;

typedef int64_t int64;
typedef uint64_t uint64;

typedef char CHAR;

#else

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

#endif // FASTC_BASE_HAS_CPP11_TYPES

#endif // _TEX_COMP_TYPES_H_
