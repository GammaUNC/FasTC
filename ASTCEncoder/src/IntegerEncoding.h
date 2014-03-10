/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#ifndef _ASTCENCODER_SRC_INTEGERENCODING_H_
#define _ASTCENCODER_SRC_INTEGERENCODING_H_

#include "TexCompTypes.h"

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
