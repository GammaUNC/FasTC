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

#include "Block.h"

#include <cassert>
#include <cstring>

namespace PVRTCC {

  Block::Block(const uint8 *data)
    : m_ColorACached(false)
    , m_ColorBCached(false) {
    assert(data);
    m_LongData = *(reinterpret_cast<const uint64 *>(data));
  }

  Pixel Block::GetColorA() {
    if(m_ColorACached) {
      return m_ColorA;
    }

    bool isOpaque = static_cast<bool>((m_LongData >> 63) & 0x1);
    const uint8 opaqueBitDepths[4] = { 0, 5, 5, 5 };
    const uint8 transBitDepths[4] = { 3, 4, 4, 4 };
    const uint8 *bitDepths = isOpaque? opaqueBitDepths : transBitDepths;

    uint8 pixelBytes[2];
    pixelBytes[0] = (m_LongData >> 56) & 0xFF;
    pixelBytes[1] = (m_LongData >> 48) & 0xFF;

    m_ColorA = Pixel(pixelBytes, bitDepths, 1);
    m_ColorACached = true;
    return m_ColorA;
  }

  Pixel Block::SetColor(const Pixel &c, bool transparent,
                        const uint8 (&tbd)[4], const uint8 (&obd)[4]) {
    uint8 cDepth[4];
    c.GetBitDepth(cDepth);

    Pixel final = c;
    if(transparent) {
      final.ChangeBitDepth(tbd);

      // If we went effectively transparent, then just switch over to opaque...
      if(final.A() == 0x7) {
        return SetColor(c, false, tbd, obd);
      }

    } else {
      final.A() = 255;
      final.ChangeBitDepth(obd);
    }

    return final;
  }

  void Block::SetColorA(const Pixel &c, bool transparent) {
    const uint8 transparentBitDepth[4] = { 3, 4, 4, 4 };
    const uint8 opaqueBitDepth[4] = { 0, 5, 5, 5 };
    m_ColorA = SetColor(c, transparent, transparentBitDepth, opaqueBitDepth);
    m_ColorACached = true;
  }

  void Block::SetColorB(const Pixel &c, bool transparent) {
    const uint8 transparentBitDepth[4] = { 3, 4, 4, 3 };
    const uint8 opaqueBitDepth[4] = { 0, 5, 5, 4 };
    m_ColorB = SetColor(c, transparent, transparentBitDepth, opaqueBitDepth);
    m_ColorBCached = true;
  }

  Pixel Block::GetColorB() {
    if(m_ColorBCached) {
      return m_ColorB;
    }

    bool isOpaque = static_cast<bool>((m_LongData >> 47) & 0x1);
    const uint8 opaqueBitDepths[4] = { 0, 5, 5, 4 };
    const uint8 transBitDepths[4] = { 3, 4, 4, 3 };
    const uint8 *bitDepths = isOpaque? opaqueBitDepths : transBitDepths;

    uint8 pixelBytes[2];
    pixelBytes[0] = (m_LongData >> 40) & 0xFF;
    pixelBytes[1] = (m_LongData >> 32) & 0xFF;

    m_ColorB = Pixel(pixelBytes, bitDepths, 1);
    m_ColorBCached = true;
    return m_ColorB;
  }

  uint8 Block::GetLerpValue(uint32 texelIdx) const {
    assert(texelIdx >= 0);
    assert(texelIdx <= 15);

    return (m_LongData >> (texelIdx * 2)) & 0x3;
  }

  void Block::SetLerpValue(uint32 texelIdx, uint8 lerpVal) {
    assert(texelIdx <= 15);

    assert(lerpVal < 4);

    m_LongData &= ~(static_cast<uint64>(0x3) << (texelIdx * 2));
    m_LongData |= static_cast<uint64>(lerpVal & 0x3) << (texelIdx * 2);
  }

  Block::E2BPPSubMode Block::Get2BPPSubMode() const {
    uint8 first = GetLerpValue(0);
    if(!(first & 0x1)) {
      return e2BPPSubMode_All;
    }

    uint8 center = GetLerpValue(10);
    if(center & 0x1) {
      return e2BPPSubMode_Vertical;
    }

    return e2BPPSubMode_Horizontal;
  }

  uint8 Block::Get2BPPLerpValue(uint32 texelIdx) const {

    if(!(GetModeBit())) {
      assert(texelIdx >= 0);
      assert(texelIdx < 32);
      return static_cast<uint8>((m_LongData >> texelIdx) & 0x1);
    }

    bool firstBitOnly = false;
    if(texelIdx == 0 ||
       (texelIdx == 10 && Get2BPPSubMode() != e2BPPSubMode_All)) {
      firstBitOnly = true;
    }

    uint8 ret = GetLerpValue(texelIdx);
    if(firstBitOnly) {
      // Change 0, 1 => 0 and 2, 3 => 3
      ret = (ret & 0x2) | ((ret >> 1) & 0x1);
    }

    return ret;
  }

  uint64 Block::Pack() {
    assert(m_ColorACached);
    assert(m_ColorBCached);

#ifndef NDEBUG
    uint8 bitDepthA[4];
    m_ColorA.GetBitDepth(bitDepthA);

    uint32 sumA = 0;
    for(int i = 0; i < 4; i++) {
      sumA += bitDepthA[i];
    }
    assert(sumA == 15);
#endif

#ifndef NDEBUG
    uint8 bitDepthB[4];
    m_ColorB.GetBitDepth(bitDepthB);

    uint32 sumB = 0;
    for(int i = 0; i < 4; i++) {
      sumB += bitDepthB[i];
    }
    assert(sumB == 14);
#endif

    uint8 aBits[2], bBits[2];
    memset(aBits, 0, sizeof(aBits));
    memset(bBits, 0, sizeof(bBits));

    m_ColorA.ToBits(aBits, 2);
    m_ColorB.ToBits(bBits, 2, 1);

    if(m_ColorA.A() == 0xFF) {
      m_ByteData[7] |= 0x80;
    } else {
      m_ByteData[7] &= 0x7f;
    }
    m_ByteData[7] = aBits[1];
    m_ByteData[6] = aBits[0];

    bool modeBit = GetModeBit();
    m_ByteData[5] = bBits[1];
    m_ByteData[4] = bBits[0];
    if(m_ColorB.A() == 0xFF) {
      m_ByteData[5] |= 0x80;
    } else {
      m_ByteData[5] &= 0x7f;
    }

    if(modeBit) {
      m_ByteData[4] |= 0x1;
    } else {
      m_ByteData[4] &= 0xFE;
    }

    // Modulation data should have already been set...
    return m_LongData;
  }

}  // namespace PVRTCC
