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

#ifndef PVRTCENCODER_SRC_BLOCK_H_
#define PVRTCENCODER_SRC_BLOCK_H_

#include "FasTC/TexCompTypes.h"
#include "FasTC/Pixel.h"

namespace PVRTCC {

using FasTC::Pixel;
class Block {
 public:
  Block(): m_LongData(0) { }
  explicit Block(const uint8 *data);

  // Accessors for the A and B colors of the block.
  Pixel GetColorA();
  void SetColorA(const Pixel &, bool transparent=false);

  Pixel GetColorB();
  void SetColorB(const Pixel &, bool transparent=false);

  bool GetModeBit() const {
    return static_cast<bool>((m_LongData >> 32) & 0x1);
  }

  void SetModeBit(bool flag) {
    const uint64 bit = 0x100000000L;
    if(flag) {
      m_LongData |= bit;
    } else {
      m_LongData &= ~bit;
    }
  }

  // For 2BPP PVRTC, if the mode bit is set, then we use the modulation data
  // as 2 bits for every other texel in the 8x4 block in a checkerboard pattern.
  // The interleaved texel data is decided by averaging nearby texel modulation
  // values. There are three different ways to average nearby texels: Either we
  // average the neighboring horizontal or vertical pixels using (a + b) / 2, or
  // we neighbor all four neighbors using (a + b + c + d + 1) / 4.
  enum E2BPPSubMode {
    e2BPPSubMode_All,
    e2BPPSubMode_Horizontal,
    e2BPPSubMode_Vertical
  };

  // For 2BPP PVRTC, this function determines the submode of the given block. The
  // submode is determined by first checking the first 2bit texel index. This texel
  // uses the high bit as a 1 bit modulation value (i.e. chooses colors A or B) and
  // the low bit is used to determine the sub-mode. If the low bit is 0, then we
  // will use e2BPPSubMode_All as defined above. If the low bit is 1, then we must
  // look at the center texel (index 10) to determine the sub-mode. In this case,
  // we treat the center texel as 1 bit modulation as well, and we use the low bit to
  // determine the sub-mode where 0 is e2BPPSubMode_Horizontal and 1 is
  // e2BPPSubMode_Vertical
  E2BPPSubMode Get2BPPSubMode() const;

  // Returns the modulation value for the texel in the 4x4 block. The texels are
  // numbered as follows:
  //  0  1  2  3
  //  4  5  6  7
  //  8  9 10 11
  // 12 13 14 15
  uint8 GetLerpValue(uint32 texelIdx) const;

  // Sets the values in the data for this block according to the texel and
  // modulation value passed. This happens immediately (i.e. a call to Pack()
  // will reflect these changes).
  void SetLerpValue(uint32 texelIdx, uint8 lerpVal);

  // This returns the modulation value for the texel in the block interpreted as
  // 2BPP. If the modulation bit is not set, then it expects a number from 0-31
  // and does the same operation as GetLerpValue. If the modulation bit is set,
  // then this function expects a number from 0-15 and returns the corresponding
  // modulation bits given the sub-mode. Note, this function does not do the
  // averaging described for E2BPPSubMode because this averaging relies on
  // global information.
  uint8 Get2BPPLerpValue(uint32 texelIdx) const;

  // Returns the 64-bit word that represents this block. This function packs the
  // A and B colors based on their bit depths and preserves the corresponding mode
  // bits. The color modes are determined by whether or not the alpha channel of
  // each block is fully opaque or not.
  uint64 Pack();

 private:
  union {
    uint8 m_ByteData[8];
    uint64 m_LongData;
  };

  bool m_ColorACached;
  Pixel m_ColorA;

  bool m_ColorBCached;
  Pixel m_ColorB;

  // tbd -- transparent bit depth
  // obd -- opaque bit depth
  static Pixel SetColor(const Pixel &c, bool transparent,
                        const uint8 (&tbd)[4], const uint8 (&obd)[4]);
};

}  // namespace PVRTCC

#endif  // PVRTCENCODER_SRC_BLOCK_H_
