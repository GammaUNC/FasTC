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
#include "IntegerEncoding.h"

#include "TexCompTypes.h"

#include "BitStream.h"
using FasTC::BitStreamReadOnly;

namespace ASTCC {

  struct TexelWeightParams {
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

      return IntegerEncodedValue::CreateEncoding(m_MaxWeight).GetBitLength(nIdxs);
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

  void DecodeColorValues(uint32 *out, uint8 *data, uint32 *modes, const uint32 nBitsForColorData) {
    // First figure out how many color values we have
    uint32 nValues = 0;
    for(uint32 i = 0; i < 4; i++) {
      nValues += ((modes[i]>>2) + 1) << 1;
    }

    // Then based on the number of values and the remaining number of bits,
    // figure out the max value for each of them...
    uint32 range = 255;
    while(range > 0) {
      IntegerEncodedValue val = IntegerEncodedValue::CreateEncoding(range);
      uint32 bitLength = val.GetBitLength(nValues);
      if(bitLength < nBitsForColorData) {
        // Find the smallest possible range that matches the given encoding
        while(--range > 0) {
          IntegerEncodedValue newval = IntegerEncodedValue::CreateEncoding(range);
          if(!newval.MatchesEncoding(val)) {
            break;
          }
        }

        // Return to last matching range.
        range++;
        break;
      }
    }

    // We now have enough to decode our integer sequence.
    std::vector<IntegerEncodedValue> decodedColorValues;
    FasTC::BitStreamReadOnly colorStream (data);
    IntegerEncodedValue::
      DecodeIntegerSequence(decodedColorValues, colorStream, range, nValues);
    assert(nValues == decodedColorValues.size());

    // Once we have the decoded values, we need to dequantize them to the 0-255 range
    // This procedure is outlined in ASTC spec C.2.13
    uint32 outIdx = 0;
    std::vector<IntegerEncodedValue>::const_iterator itr;
    for(itr = decodedColorValues.begin(); itr != decodedColorValues.end(); itr++) {
      const IntegerEncodedValue &val = *itr;
      uint32 bitlen = val.BaseBitLength();
      uint32 bitval = val.GetBitValue();

      assert(bitlen >= 1);

      uint32 A = 0, B = 0, C = 0, D = 0;
      // A is just the lsb replicated 8 times.
      for(uint32 i = 0; i < 9; i++) {
        A |= bitval & 1;
        A <<= 1;
      }

      switch(val.GetEncoding()) {
        // Replicate bits
        case eIntegerEncoding_JustBits: {
          uint32 result = bitval;
          uint32 resultLen = bitlen;
          while(resultLen < 8) {
            result <<= bitlen;
            result |= bitval & ((1 << std::min(8 - bitlen, bitlen)) - 1);
            resultLen += bitlen;
          }
          out[outIdx++] = result;
        }
        break;

        // Use algorithm in C.2.13
        case eIntegerEncoding_Trit: {

          D = val.GetTritValue();

          switch(bitlen) {
            case 1: {
              C = 204;
            }
            break;

            case 2: {
              C = 93;
              // B = b000b0bb0
              uint32 b = (bitval >> 1) & 1;
              B = (b << 8) | (b << 4) | (b << 2) | (b << 1);
            }
            break;

            case 3: {
              C = 44;
              // B = cb000cbcb
              uint32 cb = (bitval >> 1) & 3;
              B = (cb << 7) | (cb << 2) | cb;
            }
            break;

            case 4: {
              C = 22;
              // B = dcb000dcb
              uint32 dcb = (bitval >> 1) & 7;
              B = (dcb << 6) | dcb;
            }
            break;

            case 5: {
              C = 11;
              // B = edcb000ed
              uint32 edcb = (bitval >> 1) & 0xF;
              B = (edcb << 5) | (edcb >> 2);
            }
            break;

            case 6: {
              C = 5;
              // B = fedcb000f
              uint32 fedcb = (bitval >> 1) & 0x1F;
              B = (fedcb << 4) | (fedcb >> 4);
            }
            break;

            default:
              assert(!"Unsupported trit encoding for color values!");
              break;
          }  // switch(bitlen)
        }  // case eIntegerEncoding_Trit
        break;

        case eIntegerEncoding_Quint: {

          D = val.GetQuintValue();

          switch(bitlen) {
            case 1: {
              C = 113;
            }
            break;

            case 2: {
              C = 54;
              // B = b0000bb00
              uint32 b = (bitval >> 1) & 1;
              B = (b << 8) | (b << 3) | (b << 2);
            }
            break;

            case 3: {
              C = 26;
              // B = cb0000cbc
              uint32 cb = (bitval >> 1) & 3;
              B = (cb << 7) | (cb << 1) | (cb >> 1);
            }
            break;

            case 4: {
              C = 13;
              // B = dcb000dcb
              uint32 dcb = (bitval >> 1) & 7;
              B = (dcb << 6) | dcb;
            }
            break;

            case 5: {
              C = 6;
              // B = edcb0000e
              uint32 edcb = (bitval >> 1) & 0xF;
              B = (edcb << 5) | (edcb >> 3);
            }
            break;

            default:
              assert(!"Unsupported quint encoding for color values!");
              break;
          }  // switch(bitlen)
        }  // case eIntegerEncoding_Quint
        break;
      }  // switch(val.GetEncoding())

      if(val.GetEncoding() != eIntegerEncoding_JustBits) {
        uint32 T = D * C + B;
        T ^= A;
        T = (A & 0x80) | (T >> 2);
        out[outIdx++] = T;
      }
    }

    // Make sure that each of our values is in the proper range...
    for(uint32 i = 0; i < nValues; i++) {
      assert(out[i] <= 255);
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
    uint32 colorDataBits = remainingBits;
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

    // Make sure everything up till here is sane.
    for(uint32 i = 0; i < nPartitions; i++) {
      assert(colorEndpointMode[i] < 16);
    }
    assert(strm.GetBitsRead() + weightParams.GetPackedBitSize() == 128);

    // Read the texel weight data..
    uint8 texelWeightData[16];
    memset(texelWeightData, 0, sizeof(texelWeightData));
    FasTC::BitStream texelWeightStream (texelWeightData, 16*8, 0);
    
    int32 texelWeightBits = weightParams.GetPackedBitSize();
    while(texelWeightBits > 0) {
      uint32 nb = std::min(texelWeightBits, 8);
      uint32 b = strm.ReadBits(nb);
      texelWeightStream.WriteBits(b, nb);
      texelWeightBits -= 8;
    }

    assert(strm.GetBitsRead() == 128);

    // Decode both color data and texel weight data
    uint32 colorValues[32]; // Four values, two endpoints, four maximum paritions
    DecodeColorValues(colorValues, colorEndpointData, colorEndpointMode, colorDataBits);

    std::vector<IntegerEncodedValue> texelWeightValues;
    FasTC::BitStreamReadOnly weightStream (texelWeightData);
    IntegerEncodedValue::
      DecodeIntegerSequence(texelWeightValues, weightStream,
                            weightParams.m_MaxWeight,
                            weightParams.m_Width * weightParams.m_Height);
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
