/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill.
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

#include "FasTC/BPTCCompressor.h"
#include "FasTC/Shapes.h"

#include "FasTC/TexCompTypes.h"
#include "FasTC/BitStream.h"

using FasTC::BitStream;
using FasTC::BitStreamReadOnly;

#include "AnchorTables.h"
#include "CompressionMode.h"

using BPTCC::CompressionMode;

static int UnpackParams(const uint8 block[16], CompressionMode::Params &params) {
  BitStreamReadOnly strm(block);

  uint32 mode = 0;
  while(!strm.ReadBit()) {
    mode++;
  }

  const CompressionMode::Attributes *attrs =
    CompressionMode::GetAttributesForMode(mode);
  const uint32 nSubsets = attrs->numSubsets;

  params.m_IndexMode = 0;
  params.m_RotationMode = 0;
  params.m_ShapeIdx = 0;

  if ( nSubsets > 1 ) {
    params.m_ShapeIdx = strm.ReadBits(mode == 0? 4 : 6);
  } else if( attrs->hasRotation ) {
    params.m_RotationMode = strm.ReadBits(2);
    if( attrs->hasIdxMode ) {
      params.m_IndexMode = strm.ReadBit();
    }
  }

  assert(params.m_IndexMode < 2);
  assert(params.m_RotationMode < 4);
  assert(params.m_ShapeIdx < ((mode == 0)? 16U : 64U));

  uint32 cp = attrs->colorChannelPrecision;
  const uint32 shift = 8 - cp;

  uint8 eps[3][2][4];
  for(uint32 ch = 0; ch < 3; ch++)
  for(uint32 i = 0; i < nSubsets; i++)
  for(uint32 ep = 0; ep < 2; ep++)
    eps[i][ep][ch] = strm.ReadBits(cp) << shift;

  uint32 ap = attrs->alphaChannelPrecision;
  const uint32 ash = 8 - ap;

  if(ap == 0) {
    for(uint32 i = 0; i < nSubsets; i++)
    for(uint32 ep = 0; ep < 2; ep++)
      eps[i][ep][3] = 0xFF;
  } else {
    for(uint32 i = 0; i < nSubsets; i++)
    for(uint32 ep = 0; ep < 2; ep++)
      eps[i][ep][3] = strm.ReadBits(ap) << ash;
  }

  // Handle pbits
  switch(attrs->pbitType) {
    case CompressionMode::ePBitType_None:
      // Do nothing.
    break;

    case CompressionMode::ePBitType_Shared:

      cp += 1;
      ap += 1;

      for(uint32 i = 0; i < nSubsets; i++) {

        uint32 pbit = strm.ReadBit();

        for(uint32 j = 0; j < 2; j++)
        for(uint32 ch = 0; ch < kNumColorChannels; ch++) {
          const uint32 prec = ch == 3? ap : cp;
          eps[i][j][ch] |= pbit << (8-prec);
        }
      }
    break;

    case CompressionMode::ePBitType_NotShared:

      cp += 1;
      ap += 1;

      for(uint32 i = 0; i < nSubsets; i++)
      for(uint32 j = 0; j < 2; j++) {

        uint32 pbit = strm.ReadBit();

        for(uint32 ch = 0; ch < kNumColorChannels; ch++) {
          const uint32 prec = ch == 3? ap : cp;
          eps[i][j][ch] |= pbit << (8-prec);
        }
      }
    break;
  }

  // Quantize endpoints...
  for(uint32 i = 0; i < nSubsets; i++)
  for(uint32 j = 0; j < 2; j++)
  for(uint32 ch = 0; ch < kNumColorChannels; ch++) {
    const uint32 prec = ch == 3? ap : cp;
    eps[i][j][ch] |= eps[i][j][ch] >> prec;
  }

  for(uint32 i = 0; i < nSubsets; i++) {
    params.m_P1[i] = RGBAVector(eps[i][0][0], eps[i][0][1], eps[i][0][2], eps[i][0][3]);
    params.m_P2[i] = RGBAVector(eps[i][1][0], eps[i][1][1], eps[i][1][2], eps[i][1][3]);
  }

  // Figure out indices...
  uint32 alphaIndices[kMaxNumDataPoints];
  uint32 colorIndices[kMaxNumDataPoints];

  int nBitsPerAlpha = attrs->numBitsPerAlpha;
  int nBitsPerColor = attrs->numBitsPerIndex;

  uint32 idxPrec = attrs->numBitsPerIndex;
  for(uint32 i = 0; i < kMaxNumDataPoints; i++) {
    uint32 subset = BPTCC::GetSubsetForIndex(i, params.m_ShapeIdx, nSubsets);

    int idx = 0;
    if(BPTCC::GetAnchorIndexForSubset(subset, params.m_ShapeIdx, nSubsets) == i) {
      idx = strm.ReadBits(idxPrec - 1);
    } else {
      idx = strm.ReadBits(idxPrec);
    }
    colorIndices[i] = idx;
  }

  idxPrec = attrs->numBitsPerAlpha;
  if(idxPrec == 0) {
    memcpy(alphaIndices, colorIndices, sizeof(alphaIndices));
  } else {
    for(uint32 i = 0; i < kMaxNumDataPoints; i++) {
      uint32 subset = BPTCC::GetSubsetForIndex(i, params.m_ShapeIdx, nSubsets);

      int idx = 0;
      if(BPTCC::GetAnchorIndexForSubset(subset, params.m_ShapeIdx, nSubsets) == i) {
        idx = strm.ReadBits(idxPrec - 1);
      } else {
        idx = strm.ReadBits(idxPrec);
      }
      alphaIndices[i] = idx;
    }

    if(params.m_IndexMode) {
      for(uint32 i = 0; i < kMaxNumDataPoints; i++) {
        std::swap(alphaIndices[i], colorIndices[i]);
      }

      std::swap(nBitsPerAlpha, nBitsPerColor);
    }
  }

  for (uint32 i = 0; i < kMaxNumDataPoints; ++i) {
    params.m_Indices[0][i] = static_cast<uint8>(colorIndices[i]);
    params.m_AlphaIndices[i] = static_cast<uint8>(alphaIndices[i]);
  }

  assert(strm.GetBitsRead() == 128);

  return mode;
}

static FasTC::Pixel ConvertEndpoint(const int mode, const RGBAVector &ep) {

  const CompressionMode::Attributes *attrs =
    BPTCC::CompressionMode::GetAttributesForMode(mode);

  uint8 depth[4];
  memset(depth, 0, sizeof(depth));

  depth[0] = attrs->colorChannelPrecision;
  depth[1] = attrs->colorChannelPrecision;
  depth[2] = attrs->colorChannelPrecision;
  depth[3] = attrs->alphaChannelPrecision;

  if (attrs->pbitType != CompressionMode::ePBitType_None) {
    for (int i = 0; i < 4; i++) {
      if (depth[i] != 0) {
        depth[i] += 1;
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    assert(depth[i] <= 8);
  }

  FasTC::Pixel p;
  p.ChangeBitDepth(depth);

  p.R() = static_cast<int16>(ep.R()) >> (8 - depth[0]);
  p.G() = static_cast<int16>(ep.G()) >> (8 - depth[1]);
  p.B() = static_cast<int16>(ep.B()) >> (8 - depth[2]);
  if (depth[3] == 0) {
    p.A() = 0xFF;
  } else {
    p.A() = static_cast<int16>(ep.A()) >> (8 - depth[3]);
  }
  return p;
}

static void DecompressBC7Block(const uint8 block[16], BPTCC::LogicalBlock *out) {
  CompressionMode::Params params;
  int mode = UnpackParams(block, params);

  const CompressionMode::Attributes *attrs =
    BPTCC::CompressionMode::GetAttributesForMode(mode);

  BPTCC::Shape shape;
  shape.m_NumPartitions = attrs->numSubsets;
  shape.m_Index = params.m_ShapeIdx;

  out->m_Mode = static_cast<BPTCC::EBlockMode>(1 << mode);
  out->m_Shape = shape;
  for (int i = 0; i < attrs->numSubsets; ++i) {
    out->m_Endpoints[i][0] = ConvertEndpoint(mode, params.m_P1[i]);
    out->m_Endpoints[i][1] = ConvertEndpoint(mode, params.m_P2[i]);
  }

  for (uint32 i = 0; i < kMaxNumDataPoints; ++i) {
    out->m_Indices[i] = static_cast<uint32>(params.m_Indices[0][i]);
    out->m_AlphaIndices[i] = static_cast<uint32>(params.m_AlphaIndices[i]);
  }
}

static void DecompressBC7Block(const uint8 block[16], uint32 outBuf[16]) {

  CompressionMode::Params params;
  int mode = UnpackParams(block, params);

  const CompressionMode::Attributes *attrs =
    BPTCC::CompressionMode::GetAttributesForMode(mode);

  const int nBitsPerAlpha = attrs->numBitsPerAlpha;
  const int nBitsPerColor = attrs->numBitsPerIndex;

  // Get final colors by interpolating...
  for(uint32 i = 0; i < kMaxNumDataPoints; i++) {

    const uint32 subset = BPTCC::GetSubsetForIndex(i, params.m_ShapeIdx, attrs->numSubsets);
    uint32 &pixel = outBuf[i];

    pixel = 0;
    for(int ch = 0; ch < 4; ch++) {
      if(ch == 3 && nBitsPerAlpha > 0) {
        uint32 i0 =
          BPTCC::kInterpolationValues[nBitsPerAlpha - 1][params.m_AlphaIndices[i]][0];
        uint32 i1 =
          BPTCC::kInterpolationValues[nBitsPerAlpha - 1][params.m_AlphaIndices[i]][1];

        const uint32 ep1 = static_cast<uint32>(params.m_P1[subset].A());
        const uint32 ep2 = static_cast<uint32>(params.m_P2[subset].A());
        const uint8 ip = (((ep1 * i0 + ep2 * i1) + 32) >> 6) & 0xFF;
        pixel |= ip << 24;

      } else {
        uint32 i0 =
          BPTCC::kInterpolationValues[nBitsPerColor - 1][params.m_Indices[0][i]][0];
        uint32 i1 =
          BPTCC::kInterpolationValues[nBitsPerColor - 1][params.m_Indices[0][i]][1];

        const uint32 ep1 = static_cast<uint32>(params.m_P1[subset][ch]);
        const uint32 ep2 = static_cast<uint32>(params.m_P2[subset][ch]);
        const uint8 ip = (((ep1 * i0 + ep2 * i1) + 32) >> 6) & 0xFF;
        pixel |= ip << (8*ch);
      }
    }

    // Swap colors if necessary...
    uint8 *pb = reinterpret_cast<uint8 *>(&pixel);
    switch(params.m_RotationMode) {
      default:
      case 0:
        // Do nothing
        break;

      case 1:
        std::swap(pb[0], pb[3]);
        break;

      case 2:
        std::swap(pb[1], pb[3]);
        break;

      case 3:
        std::swap(pb[2], pb[3]);
        break;
    }
  }
}


namespace BPTCC {

// Convert the image from a BC7 buffer to a RGBA8 buffer
void DecompressLogical(const FasTC::DecompressionJob &dj,
                       std::vector<LogicalBlock> *out) {

  if (!out) { return; }

  out->clear();
  out->resize(dj.Height() * dj.Width() / 16);

  const uint8 *inBuf = dj.InBuf();

  int blockIdx = 0;
  for(unsigned int j = 0; j < dj.Height(); j += 4) {
    for(unsigned int i = 0; i < dj.Width(); i += 4) {
      DecompressBC7Block(inBuf, &(out->at(blockIdx++)));
      inBuf += 16;
    }
  }
}

// Convert the image from a BC7 buffer to a RGBA8 buffer
void Decompress(const FasTC::DecompressionJob &dj) {

  const uint8 *inBuf = dj.InBuf();
  uint32 *outBuf = reinterpret_cast<uint32 *>(dj.OutBuf());

  for(unsigned int j = 0; j < dj.Height(); j += 4) {
    for(unsigned int i = 0; i < dj.Width(); i += 4) {

      uint32 pixels[16];
      DecompressBC7Block(inBuf, pixels);

      memcpy(outBuf + j*dj.Width() + i, pixels, 4 * sizeof(pixels[0]));
      memcpy(outBuf + (j+1)*dj.Width() + i, pixels+4, 4 * sizeof(pixels[0]));
      memcpy(outBuf + (j+2)*dj.Width() + i, pixels+8, 4 * sizeof(pixels[0]));
      memcpy(outBuf + (j+3)*dj.Width() + i, pixels+12, 4 * sizeof(pixels[0]));

      inBuf += 16;
    }
  }
}

}  // namespace BPTCC
