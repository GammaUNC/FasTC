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

#include "FasTC/DXTCompressor.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "FasTC/Pixel.h"

namespace DXTC
{
  void DecompressDXT1Block(const uint8 *block, uint32 *outBuf) {
    // When we call FasTC::Pixel::FromBits, we expect the bits
    // to be read out of memory in LSB (byte) order first. Hence,
    // we can't read the blocks directly as uint16 values out of
    // the DXT buffer and we have to swap the bytes before hand.
    uint16 colorA = block[0];
    colorA <<= 8;
    colorA |= block[1];

    uint16 colorB = block[2];
    colorB <<= 8;
    colorB |= block[3];

    uint32 mod = reinterpret_cast<const uint32 *>(block + 4)[0];

    uint8 kFiveSixFive[4] = { 0, 5, 6, 5 };
    FasTC::Pixel a, b, c, d;
    a.FromBits(reinterpret_cast<const uint8 *>(&colorA), kFiveSixFive);
    b.FromBits(reinterpret_cast<const uint8 *>(&colorB), kFiveSixFive);

    uint8 kFullDepth[4] = {8, 8, 8, 8};
    a.ChangeBitDepth(kFullDepth);
    b.ChangeBitDepth(kFullDepth);

    d = (a + b*2) / 3;
    c = (a*2 + b) / 3;

    FasTC::Pixel *colors[4] = { &a, &b, &c, &d };

    uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
    for(uint32 i = 0; i < 16; i++) {
      outPixels[i] = colors[(mod >> (i*2)) & 3]->Pack();
    }
  }

  void DecompressDXT1(const FasTC::DecompressionJob &dcj)
  {
    assert(!(dcj.Height() & 3));
    assert(!(dcj.Width() & 3));

    uint32 blockW = dcj.Width() >> 2;
    uint32 blockH = dcj.Height() >> 2;

    const uint32 blockSz = GetBlockSize(FasTC::eCompressionFormat_DXT1);

    uint32 *outPixels = reinterpret_cast<uint32 *>(dcj.OutBuf());

    uint32 outBlock[16];
    for(uint32 j = 0; j < blockH; j++) {
      for(uint32 i = 0; i < blockW; i++) {

        uint32 offset = (j * blockW + i) * blockSz;
        DecompressDXT1Block(dcj.InBuf() + offset, outBlock);

        for(uint32 y = 0; y < 4; y++)
        for(uint32 x = 0; x < 4; x++) {
          offset = (j*4 + y)*dcj.Width() + ((i*4)+x);
          outPixels[offset] = outBlock[y*4 + x];
        }
      }
    }
  }
}
