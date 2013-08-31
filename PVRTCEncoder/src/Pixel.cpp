/* FasTC
 * Copyright (c) 2013 University of North Carolina at Chapel Hill.
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

#include "Pixel.h"

#include <cstring>
#include <cassert>

namespace PVRTCC {

  void Pixel::FromBits(const uint8 *bits,
                       const uint8 channelDepth[4],
                       uint8 bitOffset) {
    if(channelDepth) {
      memcpy(m_BitDepth, channelDepth, sizeof(m_BitDepth));
    } else {
      for(int i = 0; i < 4; i++) {
        m_BitDepth[i] = 8;
      }
    }

    uint32 nBits = bitOffset;
    for(uint32 i = 0; i < 4; i++) {
      nBits += m_BitDepth[i];
    }

    int32 byteIdx = 0;
    uint32 bitIdx = bitOffset;
    while(bitIdx >= 8) {
      bitIdx -= 8;
      byteIdx++;
    }

    for(int32 i = 0; i < 4; i++) {
      uint8 &channel = m_Component[i];
      uint32 depth = m_BitDepth[i];

      assert(depth <= 8);

      channel = 0;
      if(0 == depth) {
        channel = 0xFF;
      } else if(depth + bitIdx < 8) {
        bitIdx += depth;
        channel = (bits[byteIdx] >> (8 - bitIdx)) & ((1 << depth) - 1);
      } else {
        const uint32 numLowBits = 8 - bitIdx;
        uint32 bitsLeft = depth - numLowBits;
        channel |= bits[byteIdx] & ((1 << numLowBits) - 1);
        byteIdx++;

        const uint8 highBitsMask = ((1 << bitsLeft) - 1);
        const uint8 highBits = (bits[byteIdx] >> (8 - bitsLeft)) & highBitsMask;
        channel <<= bitsLeft;
        channel |= highBits;
        bitIdx = bitsLeft;
      }
    }
  }

  uint8 Pixel::ChangeBitDepth(uint8 val, uint8 oldDepth, uint8 newDepth) {
    assert(newDepth <= 8);
    assert(oldDepth <= 8);

    if(oldDepth == newDepth) {
      // Do nothing
      return val;
    } else if(newDepth > oldDepth) {
      uint8 bitsLeft = newDepth;
      uint8 ret = 0;
      while(bitsLeft > oldDepth) {
        ret |= val;
        ret <<= oldDepth;
        bitsLeft -= oldDepth;
      }

      return ret | (val >> (oldDepth - bitsLeft));

    } else {
      // oldDepth > newDepth
      uint8 bitsWasted = oldDepth - newDepth;
      return val >> bitsWasted;
    }

    assert(!"We shouldn't get here.");
    return 0;
  }

  void Pixel::ChangeBitDepth(const uint8 (&depth)[4]) {
    for(uint32 i = 0; i < 4; i++) {
      m_Component[i] = ChangeBitDepth(m_Component[i], m_BitDepth[i], depth[i]);
      m_BitDepth[i] = depth[i];
    }
  }

}  // namespace PVRTCC
