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
