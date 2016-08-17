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

#ifndef __BASE_INCLUDE_BITS_H__
#define __BASE_INCLUDE_BITS_H__

#include "TexCompTypes.h"

namespace FasTC {

  template<typename IntType>
  class Bits {
   private:
    const IntType &m_Bits;

    // Don't copy
    Bits() { }
    Bits(const Bits &) { }
    Bits &operator=(const Bits &) { }

   public:
    explicit Bits(IntType &v) : m_Bits(v) { }

    uint8 operator[](uint32 bitPos) {
      return static_cast<uint8>((m_Bits >> bitPos) & 1);
    }

    IntType operator()(uint32 start, uint32 end) {
      if(start == end) {
        return (*this)[start];
      } else if(start > end) {
        uint32 t = start;
        start = end;
        end = t;
      }

      uint64 mask = (1 << (end - start + 1)) - 1;
      return (m_Bits >> start) & mask;
    }
  };

  // Replicates low numBits such that [(toBit - 1):(toBit - 1 - fromBit)]
  // is the same as [(numBits - 1):0] and repeats all the way down.
  template<typename IntType>
  IntType Replicate(const IntType &val, uint32 numBits, uint32 toBit) {
    if(numBits == 0) return 0;
    if(toBit == 0) return 0;
    IntType v = val & ((1 << numBits) - 1);
    IntType res = v;
    uint32 reslen = numBits;
    while(reslen < toBit) {
      uint32 comp = 0;
      if(numBits > toBit - reslen) {
        uint32 newshift = toBit - reslen;
        comp = numBits - newshift;
        numBits = newshift;
      }
      res <<= numBits;
      res |= v >> comp;
      reslen += numBits;
    }
    return res;
  }

}  // namespace FasTC
#endif // __BASE_INCLUDE_BITS_H__
