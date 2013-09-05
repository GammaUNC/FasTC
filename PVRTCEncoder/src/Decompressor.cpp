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

#include "PVRTCCompressor.h"

#include <cassert>
#include <vector>

#include "Pixel.h"
#include "Block.h"
#include "Image.h"

namespace PVRTCC {

  static uint32 Interleave(uint16 inx, uint16 iny) {
    // Taken from:
    // http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN

    static const uint32 B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
    static const uint32 S[] = {1, 2, 4, 8};

    uint32 x = static_cast<uint32>(inx);
    uint32 y = static_cast<uint32>(iny);

    x = (x | (x << S[3])) & B[3];
    x = (x | (x << S[2])) & B[2];
    x = (x | (x << S[1])) & B[1];
    x = (x | (x << S[0])) & B[0];

    y = (y | (y << S[3])) & B[3];
    y = (y | (y << S[2])) & B[2];
    y = (y | (y << S[1])) & B[1];
    y = (y | (y << S[0])) & B[0];

    return x | (y << 1);
  }

  void Decompress(const DecompressionJob &dcj, const EWrapMode wrapMode) {
    const uint32 w = dcj.width;
    const uint32 h = dcj.height;

    assert(w > 0);
    assert(h > 0);
    assert(w % 4 == 0);
    assert(h % 4 == 0);

    // First, extract all of the block information...
    std::vector<Block> blocks;
    blocks.reserve(w * h);

    const uint32 blocksW = w / 4;
    const uint32 blocksH = h / 4;
    const uint32 blockSz = 8;

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {
        uint32 offset = (j * blocksW + i) * blockSz;
        blocks.push_back( Block(dcj.inBuf + offset) );
      }
    }

    assert(blocks.size() > 0);

    // Extract the endpoints into A and B images
    Image imgA(blocksH, blocksW);
    Image imgB(blocksH, blocksW);

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        // The blocks are initially arranged in morton order. Let's
        // linearize them... (yes I know there are faster algorithms
        // to do this out there)
        uint32 idx = Interleave(i, j);
        // uint32 idx = j * blocksW + i;
        assert(idx < static_cast<uint32>(blocks.size()));

        Block &b = blocks[idx];
        imgA(i, j) = b.GetColorA();
        imgB(i, j) = b.GetColorB();
      }
    }

    // Change the pixel mode so that all of the pixels are at the same
    // bit depth.
    const uint8 scaleDepths[4] = { 4, 5, 5, 5 };
    imgA.ChangeBitDepth(scaleDepths);
    imgB.ChangeBitDepth(scaleDepths);

    // Bilinearly upscale the images.
    imgA.BilinearUpscale(2, wrapMode);
    imgB.BilinearUpscale(2, wrapMode);

    // Change the bitdepth to full resolution
    const uint8 fullDepths[4] = { 8, 8, 8, 8 };
    imgA.ChangeBitDepth(fullDepths);
    imgB.ChangeBitDepth(fullDepths);

    // Pack the pixels based on their modulation into the resulting buffer
    // in RGBA format...
    for(uint32 j = 0; j < h; j++) {
      for(uint32 i = 0; i < w; i++) {
        const uint32 blockIdx = (j/4) * blocksW + (i / 4);
        const Block &b = blocks[blockIdx];

        const uint32 texelIndex = (j % 4) * 4 + (i % 4);
        const Pixel &pa = imgA(i, j);
        const Pixel &pb = imgB(i, j);

        Pixel result;
        if(b.GetModeBit()) {
          const uint8 lerpVals[3] = { 0, 4, 8 };
          uint8 modVal = b.GetLerpValue(texelIndex);
          bool punchThrough = false;

          if(modVal == 2) {
            modVal = 1;
            punchThrough = true;
          } else if(modVal == 3) {
            modVal = 2;
          }

          const uint8 lerpVal = lerpVals[modVal];

          for(uint32 c = 0; c < 4; c++) {
            int16 va = static_cast<int16>(pa.Component(c));
            int16 vb = static_cast<int16>(pb.Component(c));

            result.Component(c) = va + ((vb - va) * lerpVal) / 8;
          }

          if(punchThrough) {
            result.A() = 0;
          }

        } else {
          const uint8 lerpVals[4] = { 8, 5, 3, 0 };
          const uint8 lerpVal = lerpVals[b.GetLerpValue(texelIndex)];

          for(uint32 c = 0; c < 4; c++) {
            int16 va = static_cast<int16>(pa.Component(c));
            int16 vb = static_cast<int16>(pb.Component(c));

            result.Component(c) = (va * (8 - lerpVal) + vb * lerpVal) / 8;
          }
        }

        uint32 *outPixels = reinterpret_cast<uint32 *>(dcj.outBuf);
        outPixels[(j * h) + i] = result.PackRGBA();
      }
    }
  }

}  // namespace PVRTCC
