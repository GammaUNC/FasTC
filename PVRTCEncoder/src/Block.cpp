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

}  // namespace PVRTCC
