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

#include "ASTCCompressor.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "Utils.h"

#include "TexCompTypes.h"

#include "Bits.h"
using FasTC::Bits;

#include "BitStream.h"
using FasTC::BitStreamReadOnly;

namespace ASTCC {

  // According to table C.2.7
  void GetBitEncoding(uint8 &nQuints, uint8 &nTrits, uint8 &nBits,
                      const uint32 maxWeight) {
    nQuints = nTrits = nBits = 0;
    switch(maxWeight) {
    case 1: nBits = 1; return;
    case 2: nTrits = 1; return;
    case 3: nBits = 2; return;
    case 4: nQuints = 1; return;
    case 5: nTrits = 1; nBits = 1; return;
    case 7: nBits = 3; return;
    case 9: nQuints = 1; nBits = 1; return;
    case 11: nTrits = 1; nBits = 2; return;
    case 15: nBits = 4; return;
    case 19: nQuints = 1; nBits = 2; return;
    case 23: nTrits = 1; nBits = 3; return;
    case 31: nBits = 5; return;

    default:
      assert(!"Invalid maximum weight");
      return;
    }
  }

  class TexelWeightParams {
   public:
    uint32 m_Width;
    uint32 m_Height;
    bool m_bDualPlane;
    uint32 m_MaxWeight;
    bool m_bError;
    bool m_bVoidExtent;

    TexelWeightParams() {
      memset(this, 0, sizeof(*this));
    }

    uint32 GetPackedBitSize() {
      // How many indices do we have?
      uint32 nIdxs = m_Height * m_Width;
      if(m_bDualPlane) {
        nIdxs *= 2;
      }

      // How are they encoded?
      uint8 nQuints, nTrits, nBits;
      GetBitEncoding(nQuints, nTrits, nBits, m_MaxWeight);

      // nQuints and nTrits are mutually exclusive values of one.
      assert(nQuints != 1 || nTrits == 0);
      assert(nTrits != 1 || nQuints == 0);

      // each index has at least as many bits per index as described.
      uint32 totalBits = nBits * nIdxs;
      totalBits += (nIdxs * 8 * nTrits + 4) / 5;
      totalBits += (nIdxs * 7 * nQuints + 2) / 3;
      return totalBits;
    }
  };

  TexelWeightParams DecodeBlockInfo(BitStreamReadOnly &strm) {
    TexelWeightParams params;

    // Read the entire block mode all at once
    uint16 modeBits = strm.ReadBits(11);

    // Does this match the void extent block mode?
    if((modeBits & 0x01FF) == 0x1FC) {
      params.m_bVoidExtent = true;
      return params;
    }

    // First check if the last four bits are zero
    if((modeBits & 0xF) != 0) {
      params.m_bError = true;
      return params;
    }

    // If the last two bits are zero, then if bits
    // [6-8] are all ones, this is also reserved.
    if((modeBits & 0x3) != 0 &&
       (modeBits & 0x1C0) == 0x1C0) {
      params.m_bError = true;
      return params;
    }

    // Otherwise, there is no error... Figure out the layout
    // of the block mode. Layout is determined by a number
    // between 0 and 9 corresponding to table C.2.8 of the
    // ASTC spec.
    uint32 layout = 0;

    if((modeBits & 0x1) || (modeBits & 0x2)) {
      // layout is in [0-4]
      if(modeBits & 0x8) {
        // layout is in [2-4]
        if(modeBits & 0x4) {
          // layout is in [3-4]
          if(modeBits & 0x100) {
            layout = 4;
          } else {
            layout = 3;
          }
        } else {
          layout = 2;
        }
      } else {
        // layout is in [0-1]
        if(modeBits & 0x4) {
          layout = 1;
        } else {
          layout = 0;
        }
      }
    } else {
      // layout is in [5-9]
      if(modeBits & 0x100) {
        // layout is in [7-9]
        if(modeBits & 0x80) {
          // layout is in [7-8]
          assert(modeBits & 0x40 == 0);
          if(modeBits & 0x20) {
            layout = 8;
          } else {
            layout = 7;
          }
        } else {
          layout = 9;
        }
      } else {
        // layout is in [5-6]
        if(modeBits & 0x80) {
          layout = 6;
        } else {
          layout = 5;
        }
      }
    }

    assert(layout < 10);

    // Determine R
    uint32 R = !!(modeBits & 0x10);
    if(layout < 4) {
      R |= (modeBits & 0x3) << 1;
    } else {
      R |= (modeBits & 0xC) >> 1;
    }
    assert(2 <= R && R <= 7);

    // Determine width & height
    switch(layout) {
      case 0: {
        uint32 A = (modeBits >> 5) & 0x3;
        uint32 B = (modeBits >> 7) & 0x3;
        params.m_Width = B + 4;
        params.m_Height = A + 2;
        break;
      }
  
      case 1: {
        uint32 A = (modeBits >> 5) & 0x3;
        uint32 B = (modeBits >> 7) & 0x3;
        params.m_Width = B + 8;
        params.m_Height = A + 2;
        break;
      }
  
      case 2: {
        uint32 A = (modeBits >> 5) & 0x3;
        uint32 B = (modeBits >> 7) & 0x3;
        params.m_Width = A + 2;
        params.m_Height = B + 8;
        break;
      }
  
      case 3: {
        uint32 A = (modeBits >> 5) & 0x3;
        uint32 B = (modeBits >> 7) & 0x1;
        params.m_Width = A + 2;
        params.m_Height = B + 6;
        break;
      }
  
      case 4: {
        uint32 A = (modeBits >> 5) & 0x3;
        uint32 B = (modeBits >> 7) & 0x1;
        params.m_Width = B + 2;
        params.m_Height = A + 2;
        break;
      }
  
      case 5: {
        uint32 A = (modeBits >> 5) & 0x3;
        params.m_Width = 12;
        params.m_Height = A + 2;
        break;
      }
  
      case 6: {
        uint32 A = (modeBits >> 5) & 0x3;
        params.m_Width = A + 2;
        params.m_Height = 12;
        break;
      }
  
      case 7: {
        params.m_Width = 6;
        params.m_Height = 10;
        break;
      }
  
      case 8: {
        params.m_Width = 10;
        params.m_Height = 6;
        break;
      }
  
      case 9: {
        uint32 A = (modeBits >> 5) & 0x3;
        uint32 B = (modeBits >> 9) & 0x3;
        params.m_Width = A + 6;
        params.m_Height = B + 6;
        break;
      }
  
      default:
        assert(!"Don't know this layout...");
        params.m_bError = true;
        break;
    }

    // Determine whether or not we're using dual planes
    // and/or high precision layouts.
    bool D = (layout != 9) && (modeBits & 0x400);
    bool H = (layout != 9) && (modeBits & 0x200);

    if(H) {
      const uint32 maxWeights[6] = { 9, 11, 15, 19, 23, 31 };
      params.m_MaxWeight = maxWeights[R-2];
    } else {
      const uint32 maxWeights[6] = { 1, 2, 3, 4, 5, 7 };
      params.m_MaxWeight = maxWeights[R-2];
    }

    params.m_bDualPlane = D;

    return params;
  }

  void FillError(uint8 *outBuf, uint32 blockWidth, uint32 blockHeight) {
    for(uint32 j = 0; j < blockHeight; j++)
    for(uint32 i = 0; i < blockWidth; i++) {
      reinterpret_cast<uint32 *>(outBuf)[j * blockWidth + i] = 0xFFFF00FF;
    }
  }

  void DecodeTritBlock(BitStreamReadOnly &bits,
                       std::vector<uint32> &result,
                       uint32 nBitsPerValue) {
    // Implement the algorithm in section C.2.12
    uint32 m[5];
    uint32 t[5];
    uint32 T;

    // Read the trit encoded block according to
    // table C.2.14
    m[0] = bits.ReadBits(nBitsPerValue);
    T = bits.ReadBits(2);
    m[1] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits(2) << 2;
    m[2] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 4;
    m[3] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBits(2) << 5;
    m[4] = bits.ReadBits(nBitsPerValue);
    T |= bits.ReadBit() << 7;

    uint32 C = 0;

    Bits<uint32> Tb(T);
    if(Tb(2, 4) == 7) {
      C = (Tb(5, 7) << 2) | Tb(0, 1);
      t[4] = t[3] = 2;
    } else {
      C = Tb(0, 4);
      if(Tb(5, 6) == 3) {
        t[4] = 2;
        t[3] = Tb[7];
      } else {
        t[4] = Tb[7];
        t[3] = Tb(5, 6);
      }
    }

    Bits<uint32> Cb(C);
    if(Cb(0, 1) == 3) {
      t[2] = 2;
      t[1] = Cb[4];
      t[0] = (Cb[3] << 1) | (Cb[2] & ~Cb[3]);
    } else if(Cb(2, 3) == 3) {
      t[2] = 2;
      t[1] = 2;
      t[0] = Cb(0, 1);
    } else {
      t[2] = Cb[4];
      t[1] = Cb(2, 3);
      t[0] = (Cb[1] << 1) | (Cb[0] & ~Cb[1]);
    }

    for(uint32 i = 0; i < 5; i++) {
      assert(t[i] < 3);
      uint32 val = (t[i] << nBitsPerValue) + m[i];
      result.push_back(val);
    }
  }

  void DecodeQuintBlock(BitStreamReadOnly &bits,
                        std::vector<uint32> &result,
                        uint32 nBitsPerValue) {
    // Implement the algorithm in section C.2.12
    uint32 m[3];
    uint32 q[3];
    uint32 Q;

    // Read the trit encoded block according to
    // table C.2.15
    m[0] = bits.ReadBits(nBitsPerValue);
    Q = bits.ReadBits(3);
    m[1] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits(2) << 3;
    m[2] = bits.ReadBits(nBitsPerValue);
    Q |= bits.ReadBits(2) << 5;

    Bits<uint32> Qb(Q);
    if(Qb(1, 2) == 3 && Qb(5, 6) == 0) {
      q[0] = q[1] = 4;
      q[2] = (Qb[0] << 2) | ((Qb[4] & ~Qb[0]) << 1) | (Qb[3] & ~Qb[0]);
    } else {
      uint32 C = 0;
      if(Qb(1, 2) == 3) {
        q[2] = 4;
        C = (Qb(3, 4) << 3) | ((~Qb(5, 6) & 3) << 1) | Qb[0];
      } else {
        q[2] = Qb(5, 6);
        C = Qb(0, 4);
      }

      Bits<uint32> Cb(C);
      if(Cb(0, 2) == 5) {
        q[1] = 4;
        q[0] = Cb(3, 4);
      } else {
        q[1] = Cb(3, 4);
        q[0] = Cb(0, 2);
      }
    }

    for(uint32 i = 0; i < 3; i++) {
      assert(q[i] < 5);
      uint32 val = (q[i] << nBitsPerValue) + m[i];
      result.push_back(val);
    }
  }

  void DecodeIntegerSequence(BitStreamReadOnly &bits,
                             std::vector<uint32> &result,
                             uint32 maxRange,
                             uint32 nValues) {
    // Clean our result vector
    result.clear();
    result.reserve(nValues);

    // Determine encoding parameters
    uint8 nQuints, nTrits, nBits;
    GetBitEncoding(nQuints, nTrits, nBits, maxRange);

    // Start decoding
    uint32 nValsDecoded = 0;
    while(nValsDecoded < nValues) {
      if(nQuints) {
        DecodeQuintBlock(bits, result, nBits);
        nValsDecoded += 3;
      } else if(nTrits) {
        DecodeTritBlock(bits, result, nBits);
        nValsDecoded += 5;
      } else {
        // Decode bit by bit
        result.push_back(bits.ReadBits(nBits));
        nValsDecoded++;
      }
    }
  }

  void DecompressBlock(const uint8 inBuf[16],
                       const uint32 blockWidth, const uint32 blockHeight,
                       uint8 *outBuf) {
    BitStreamReadOnly strm(inBuf);
    TexelWeightParams weightParams = DecodeBlockInfo(strm);
    
    // Was there an error?
    if(weightParams.m_bError) {
      assert(!"Invalid block mode");
      FillError(outBuf, blockWidth, blockHeight);
      return;
    }

    if(weightParams.m_Width > blockWidth) {
      assert(!"Texel weight grid width should be smaller than block width");
      FillError(outBuf, blockWidth, blockHeight);
      return;
    }

    if(weightParams.m_Height > blockHeight) {
      assert(!"Texel weight grid height should be smaller than block height");
      FillError(outBuf, blockWidth, blockHeight);
      return;
    }

    // Read num partitions
    uint32 nPartitions = strm.ReadBits(2) + 1;
    assert(nPartitions <= 4);

    if(nPartitions == 4 && weightParams.m_bDualPlane) {
      assert(!"Dual plane mode is incompatible with four partition blocks");
      FillError(outBuf, blockWidth, blockHeight);
      return;
    }

    // Based on the number of partitions, read the color endpoint mode for
    // each partition.

    // Determine partitions, partition index, and color endpoint modes
    int32 planeIdx = -1;
    uint32 partitionIndex = nPartitions;
    uint32 colorEndpointMode[4] = {0, 0, 0, 0};
 
    // Define color data.
    uint8 colorEndpointData[16];
    memset(colorEndpointData, 0, sizeof(colorEndpointData));
    FasTC::BitStream colorEndpointStream (colorEndpointData, 16*8, 0);

    // Read extra config data...
    uint32 baseCEM = 0;
    if(nPartitions == 1) {
      colorEndpointMode[0] = strm.ReadBits(4);
    } else {
      uint32 restOfPartitionIndex = strm.ReadBits(10);
      partitionIndex |= restOfPartitionIndex << 2;
      baseCEM = strm.ReadBits(6);
    }
    uint32 baseMode = (baseCEM & 3);

    // Remaining bits are color endpoint data...
    uint32 nWeightBits = weightParams.GetPackedBitSize();
    int32 remainingBits = 128 - nWeightBits - strm.GetBitsRead();

    // Consider extra bits prior to texel data...
    uint32 extraCEMbits = 0;
    if(baseMode) {
      switch(nPartitions) {
      case 2: extraCEMbits += 2; break;
      case 3: extraCEMbits += 5; break;
      case 4: extraCEMbits += 8; break;
      default: assert(false); break;
      }
    }
    remainingBits -= extraCEMbits;

    // Do we have a dual plane situation?
    uint32 planeSelectorBits = 0;
    if(weightParams.m_bDualPlane) {
      planeSelectorBits = 2;
    }
    remainingBits -= planeSelectorBits;

    // Read color data...
    while(remainingBits > 0) {
      uint32 nb = std::min(remainingBits, 8);
      uint32 b = strm.ReadBits(nb);
      colorEndpointStream.WriteBits(b, nb);
      remainingBits -= 8;
    }

    // Read the plane selection bits
    planeIdx = strm.ReadBits(planeSelectorBits);

    // Read the rest of the CEM
    if(baseMode) {
      uint32 extraCEM = strm.ReadBits(extraCEMbits);
      uint32 CEM = (extraCEM << 6) | baseCEM;
      CEM >>= 2;

      bool C[4] = { 0 };
      for(uint32 i = 0; i < nPartitions; i++) {
        C[i] = CEM & 1;
        CEM >>= 1;
      }

      uint8 M[4] = { 0 };
      for(uint32 i = 0; i < nPartitions; i++) {
        M[i] = CEM & 3;
        CEM >>= 2;
        assert(M[i] <= 3);
      }

      for(uint32 i = 0; i < nPartitions; i++) {
        colorEndpointMode[i] = baseMode;
        if(!(C[i])) colorEndpointMode[i] -= 1;
        colorEndpointMode[i] <<= 2;
        colorEndpointMode[i] |= M[i];
      }
    } else if(nPartitions > 1) {
      uint32 CEM = baseCEM >> 2;
      for(uint32 i = 0; i < nPartitions; i++) {
        colorEndpointMode[i] = CEM;
      }
    }

#ifndef NDEBUG
    // Make sure everything up till here is sane.
    for(uint32 i = 0; i < nPartitions; i++) {
      assert(colorEndpointMode[i] < 16);
    }
    assert(strm.GetBitsRead() + weightParams.GetPackedBitSize() == 128);
#endif

    // Read the texel weight data..
    
  }

  void Decompress(const FasTC::DecompressionJob &dcj, EASTCBlockSize blockSize) {
    uint32 blockWidth = GetBlockWidth(blockSize);
    uint32 blockHeight = GetBlockHeight(blockSize);
    uint32 blockIdx = 0;
    for(uint32 j = 0; j < dcj.Width(); j++) {
      for(uint32 i = 0; i < dcj.Height(); i++) {

        const uint8 *blockPtr = dcj.InBuf() + blockIdx*16;

        uint32 uncompData[144];
        uint8 *dataPtr = reinterpret_cast<uint8 *>(uncompData);
        DecompressBlock(blockPtr, blockWidth, blockHeight, dataPtr);

        uint8 *outRow = dcj.OutBuf() + (j*dcj.Width() + i)*4;
        for(uint32 jj = 0; jj < blockHeight; jj++) {
          memcpy(outRow + jj*dcj.Width()*4, uncompData + jj*blockWidth, blockWidth*4);
        }

        blockIdx++;
      }
    }
  }

}  // namespace ASTCC
