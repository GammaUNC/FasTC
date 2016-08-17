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

#ifndef _ASTCENCODER_SRC_INTEGERENCODING_H_
#define _ASTCENCODER_SRC_INTEGERENCODING_H_

#include "FasTC/TexCompTypes.h"

// Forward declares
namespace FasTC {
  class BitStreamReadOnly;
}

namespace ASTCC {

  enum EIntegerEncoding {
    eIntegerEncoding_JustBits,
    eIntegerEncoding_Quint,
    eIntegerEncoding_Trit
  };

  class IntegerEncodedValue {
   private:
    const EIntegerEncoding m_Encoding;
    const uint32 m_NumBits;
    uint32 m_BitValue;
    union {
      uint32 m_QuintValue;
      uint32 m_TritValue;
    };

   public:

    // Jank, but we're not doing any heavy lifting in this class, so it's
    // probably OK. It allows us to use these in std::vectors...
    IntegerEncodedValue &operator=(const IntegerEncodedValue &other) {
      new (this) IntegerEncodedValue(other);
      return *this;
    }

    IntegerEncodedValue(EIntegerEncoding encoding, uint32 numBits)
      : m_Encoding(encoding), m_NumBits(numBits) { }

    EIntegerEncoding GetEncoding() const { return m_Encoding; }
    uint32 BaseBitLength() const { return m_NumBits; }

    uint32 GetBitValue() const { return m_BitValue; }
    void SetBitValue(uint32 val) { m_BitValue = val; }

    uint32 GetTritValue() const { return m_TritValue; }
    void SetTritValue(uint32 val) { m_TritValue = val; }

    uint32 GetQuintValue() const { return m_QuintValue; }
    void SetQuintValue(uint32 val) { m_QuintValue = val; }

    bool MatchesEncoding(const IntegerEncodedValue &other) {
      return m_Encoding == other.m_Encoding && m_NumBits == other.m_NumBits;
    }

    // Returns the number of bits required to encode nVals values.
    uint32 GetBitLength(uint32 nVals);

    // Returns the value of this integer encoding.
    uint32 GetValue();

    // Returns a new instance of this struct that corresponds to the
    // can take no more than maxval values
    static IntegerEncodedValue CreateEncoding(uint32 maxVal);

    // Fills result with the values that are encoded in the given
    // bitstream. We must know beforehand what the maximum possible
    // value is, and how many values we're decoding.
    static void DecodeIntegerSequence(
      std::vector<IntegerEncodedValue> &result,
      FasTC::BitStreamReadOnly &bits,
      uint32 maxRange,
      uint32 nValues
    );

   private:
    static void DecodeTritBlock(
      FasTC::BitStreamReadOnly &bits,
      std::vector<IntegerEncodedValue> &result,
      uint32 nBitsPerValue
    );
    static void DecodeQuintBlock(
      FasTC::BitStreamReadOnly &bits,
      std::vector<IntegerEncodedValue> &result,
      uint32 nBitsPerValue
    );
  };
};  // namespace ASTCC

#endif  //  _ASTCENCODER_SRC_INTEGERENCODING_H_
