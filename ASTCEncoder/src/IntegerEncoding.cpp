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

#include "FasTC/ASTCCompressor.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "FasTC/Bits.h"
using FasTC::Bits;

#include "FasTC/BitStream.h"
using FasTC::BitStreamReadOnly;

#include "Utils.h"
#include "IntegerEncoding.h"

namespace ASTCC {

  // Returns the number of bits required to encode nVals values.
  uint32 IntegerEncodedValue::GetBitLength(uint32 nVals) {
    uint32 totalBits = m_NumBits * nVals;
    if(m_Encoding == eIntegerEncoding_Trit) {
      totalBits += (nVals * 8 + 4) / 5;
    } else if(m_Encoding == eIntegerEncoding_Quint) {
      totalBits += (nVals * 7 + 2) / 3;
    }
    return totalBits;
  }

  IntegerEncodedValue IntegerEncodedValue::CreateEncoding(uint32 maxVal) {
    while(maxVal > 0) {
      uint32 check = maxVal + 1;

      // Is maxVal a power of two?
      if(!(check & (check - 1))) {
        return IntegerEncodedValue(eIntegerEncoding_JustBits, Popcnt(maxVal));
      }

      // Is maxVal of the type 3*2^n - 1?
      if((check % 3 == 0) && !((check/3) & ((check/3) - 1))) {
        return IntegerEncodedValue(eIntegerEncoding_Trit, Popcnt(check/3 - 1));
      }

      // Is maxVal of the type 5*2^n - 1?
      if((check % 5 == 0) && !((check/5) & ((check/5) - 1))) {
        return IntegerEncodedValue(eIntegerEncoding_Quint, Popcnt(check/5 - 1));
      }

      // Apparently it can't be represented with a bounded integer sequence...
      // just iterate.
      maxVal--;
    }
    return IntegerEncodedValue(eIntegerEncoding_JustBits, 0);
  }

  uint32 IntegerEncodedValue::GetValue() {
    switch(m_Encoding) {
      case eIntegerEncoding_JustBits:
        return m_BitValue;

      case eIntegerEncoding_Trit:
        return (m_TritValue << m_NumBits) + m_BitValue;

      case eIntegerEncoding_Quint: 
        return (m_QuintValue << m_NumBits) + m_BitValue;
    }

    return 0;
  }

  void IntegerEncodedValue::DecodeTritBlock(
    BitStreamReadOnly &bits,
    std::vector<IntegerEncodedValue> &result,
    uint32 nBitsPerValue
  ) {
    // Implement the algorithm in section C.2.12
    uint32 m[5];
    uint32 t[5];
    uint32 T;

    // Read the trit encoded block according to
    // table C.2.14
    m[0] = bits.ReadBits(nBitsPerValue);
    T = bits.ReadBits(2);
    m[1] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits(2) << 2;
    m[2] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 4;
    m[3] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits(2) << 5;
    m[4] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 7;

    uint32 C = 0;

    Bits<uint32> Tb(T);
    if(Tb(2, 4) == 7) {
      C = (Tb(5, 7) << 2) | Tb(0, 1);
      t[4] = t[3] = 2;
    } else {
      C = Tb(0, 4);
      if(Tb(5, 6) == 3) {
        t[4] = 2;
        t[3] = Tb[7];
      } else {
        t[4] = Tb[7];
        t[3] = Tb(5, 6);
      }
    }

    Bits<uint32> Cb(C);
    if(Cb(0, 1) == 3) {
      t[2] = 2;
      t[1] = Cb[4];
      t[0] = (Cb[3] << 1) | (Cb[2] & ~Cb[3]);
    } else if(Cb(2, 3) == 3) {
      t[2] = 2;
      t[1] = 2;
      t[0] = Cb(0, 1);
    } else {
      t[2] = Cb[4];
      t[1] = Cb(2, 3);
      t[0] = (Cb[1] << 1) | (Cb[0] & ~Cb[1]);
    }

    for(uint32 i = 0; i < 5; i++) {
      IntegerEncodedValue val(eIntegerEncoding_Trit, nBitsPerValue);
      val.SetBitValue(m[i]);
      val.SetTritValue(t[i]);
      result.push_back(val);
    }
  }

  void IntegerEncodedValue::DecodeQuintBlock(
    BitStreamReadOnly &bits,
    std::vector<IntegerEncodedValue> &result,
    uint32 nBitsPerValue
  ) {
    // Implement the algorithm in section C.2.12
    uint32 m[3];
    uint32 q[3];
    uint32 Q;

    // Read the trit encoded block according to
    // table C.2.15
    m[0] = bits.ReadBits(nBitsPerValue);
    Q = bits.ReadBits(3);
    m[1] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits(2) << 3;
    m[2] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits(2) << 5;

    Bits<uint32> Qb(Q);
    if(Qb(1, 2) == 3 && Qb(5, 6) == 0) {
      q[0] = q[1] = 4;
      q[2] = (Qb[0] << 2) | ((Qb[4] & ~Qb[0]) << 1) | (Qb[3] & ~Qb[0]);
    } else {
      uint32 C = 0;
      if(Qb(1, 2) == 3) {
        q[2] = 4;
        C = (Qb(3, 4) << 3) | ((~Qb(5, 6) & 3) << 1) | Qb[0];
      } else {
        q[2] = Qb(5, 6);
        C = Qb(0, 4);
      }

      Bits<uint32> Cb(C);
      if(Cb(0, 2) == 5) {
        q[1] = 4;
        q[0] = Cb(3, 4);
      } else {
        q[1] = Cb(3, 4);
        q[0] = Cb(0, 2);
      }
    }

    for(uint32 i = 0; i < 3; i++) {
      IntegerEncodedValue val(eIntegerEncoding_Quint, nBitsPerValue);
      val.m_BitValue = m[i];
      val.m_QuintValue = q[i];
      result.push_back(val);
    }
  }

  void IntegerEncodedValue::DecodeIntegerSequence(
    std::vector<IntegerEncodedValue> &result,
    BitStreamReadOnly &bits,
    uint32 maxRange,
    uint32 nValues
  ) {
    // Determine encoding parameters
    IntegerEncodedValue val = IntegerEncodedValue::CreateEncoding(maxRange);

    // Start decoding
    uint32 nValsDecoded = 0;
    while(nValsDecoded < nValues) {
      switch(val.GetEncoding()) {
        case eIntegerEncoding_Quint:
          DecodeQuintBlock(bits, result, val.BaseBitLength());
          nValsDecoded += 3;
          break;

        case eIntegerEncoding_Trit:
          DecodeTritBlock(bits, result, val.BaseBitLength());
          nValsDecoded += 5;
          break;

        case eIntegerEncoding_JustBits:
          val.SetBitValue(bits.ReadBits(val.BaseBitLength()));
          result.push_back(val);
          nValsDecoded++;
          break;
      }
    }
  }
}  // namespace ASTCC
