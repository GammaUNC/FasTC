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

#include "Bits.h"
#include "BitStream.h"
using FasTC::BitStreamReadOnly;

#include "Pixel.h"

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

    uint32 GetNumWeightValues() const {
      uint32 ret = m_Width * m_Height;
      if(m_bDualPlane) {
        ret *= 2;
      }
      return ret;
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
    if((modeBits & 0xF) == 0) {
      params.m_bError = true;
      return params;
    }

    // If the last two bits are zero, then if bits
    // [6-8] are all ones, this is also reserved.
    if((modeBits & 0x3) == 0 &&
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
          assert((modeBits & 0x40) == 0U);
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

  void FillError(uint32 *outBuf, uint32 blockWidth, uint32 blockHeight) {
    for(uint32 j = 0; j < blockHeight; j++)
    for(uint32 i = 0; i < blockWidth; i++) {
      outBuf[j * blockWidth + i] = 0xFFFF00FF;
    }
  }

  void DecodeColorValues(uint32 *out, uint8 *data, uint32 *modes,
                         const uint32 nPartitions, const uint32 nBitsForColorData) {
    // First figure out how many color values we have
    uint32 nValues = 0;
    for(uint32 i = 0; i < nPartitions; i++) {
      nValues += ((modes[i]>>2) + 1) << 1;
    }

    // Then based on the number of values and the remaining number of bits,
    // figure out the max value for each of them...
    uint32 range = 256;
    while(--range > 0) {
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

    // Once we have the decoded values, we need to dequantize them to the 0-255 range
    // This procedure is outlined in ASTC spec C.2.13
    uint32 outIdx = 0;
    std::vector<IntegerEncodedValue>::const_iterator itr;
    for(itr = decodedColorValues.begin(); itr != decodedColorValues.end(); itr++) {
      // Have we already decoded all that we need?
      if(outIdx >= nValues) {
        break;
      }

      const IntegerEncodedValue &val = *itr;
      uint32 bitlen = val.BaseBitLength();
      uint32 bitval = val.GetBitValue();

      assert(bitlen >= 1);

      uint32 A = 0, B = 0, C = 0, D = 0;
      // A is just the lsb replicated 9 times.
      A = FasTC::Replicate(bitval & 1, 1, 9);

      switch(val.GetEncoding()) {
        // Replicate bits
        case eIntegerEncoding_JustBits: 
          out[outIdx++] = FasTC::Replicate(bitval, bitlen, 8);
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

  uint32 UnquantizeTexelWeight(const IntegerEncodedValue &val) {
    uint32 bitval = val.GetBitValue();
    uint32 bitlen = val.BaseBitLength();

    uint32 A = FasTC::Replicate(bitval & 1, 1, 7);
    uint32 B = 0, C = 0, D = 0;

    uint32 result = 0;
    switch(val.GetEncoding()) {
      case eIntegerEncoding_JustBits:
        result = FasTC::Replicate(bitval, bitlen, 6);
      break;

      case eIntegerEncoding_Trit: {
        D = val.GetTritValue();
        assert(D < 3);

        switch(bitlen) {
          case 0: {
            uint32 results[3] = { 0, 32, 63 };
            result = results[D];
          }
          break;

          case 1: {
            C = 50;
          }
          break;

          case 2: {
            C = 23;
            uint32 b = (bitval >> 1) & 1;
            B = (b << 6) | (b << 2) | b;
          }
          break;

          case 3: {
            C = 11;
            uint32 cb = (bitval >> 1) & 3;
            B = (cb << 5) | cb;
          }
          break;

          default:
            assert(!"Invalid trit encoding for texel weight");
            break;
        }
      }
      break;

      case eIntegerEncoding_Quint: {
        D = val.GetQuintValue();
        assert(D < 5);

        switch(bitlen) {
          case 0: {
            uint32 results[5] = { 0, 16, 32, 47, 63 };
            result = results[D];
          }
          break;

          case 1: {
            C = 28;
          }
          break;

          case 2: {
            C = 13;
            uint32 b = (bitval >> 1) & 1;
            B = (b << 6) | (b << 1);
          }
          break;

          default:
            assert(!"Invalid quint encoding for texel weight");
            break;
        }
      }
      break;
    }

    if(val.GetEncoding() != eIntegerEncoding_JustBits && bitlen > 0) {
      // Decode the value...
      result = D * C + B;
      result ^= A;
      result = (A & 0x20) | (result >> 2);
    }

    assert(result < 64);

    // Change from [0,63] to [0,64]
    if(result > 32) {
      result += 1;
    }

    return result;
  }

  void UnquantizeTexelWeights(uint32 out[2][144],
                              std::vector<IntegerEncodedValue> &weights,
                              const TexelWeightParams &params,
                              const uint32 blockWidth, const uint32 blockHeight) {
    uint32 weightIdx = 0;
    uint32 unquantized[2][144];
    std::vector<IntegerEncodedValue>::const_iterator itr;
    for(itr = weights.begin(); itr != weights.end(); itr++) {
      unquantized[0][weightIdx] = UnquantizeTexelWeight(*itr);

      if(params.m_bDualPlane) {
        itr++;
        unquantized[1][weightIdx] = UnquantizeTexelWeight(*itr);
        if(itr == weights.end()) {
          break;
        }
      }

      if(++weightIdx >= (params.m_Width * params.m_Height))
        break;
    }

    // Do infill if necessary (Section C.2.18) ...
    uint32 Ds = (1024 + (blockWidth/2)) / (blockWidth - 1);
    uint32 Dt = (1024 + (blockHeight/2)) / (blockHeight - 1);

    for(uint32 plane = 0; plane < (params.m_bDualPlane? 2 : 1); plane++)
    for(uint32 t = 0; t < blockHeight; t++)
    for(uint32 s = 0; s < blockWidth; s++) {
      uint32 cs = Ds * s;
      uint32 ct = Dt * t;

      uint32 gs = (cs * (params.m_Width - 1) + 32) >> 6;
      uint32 gt = (ct * (params.m_Height - 1) + 32) >> 6;

      uint32 js = gs >> 4;
      uint32 fs = gs & 0xF;

      uint32 jt = gt >> 4;
      uint32 ft = gt & 0x0F;

      uint32 w11 = (fs * ft + 8) >> 4;
      uint32 w10 = ft - w11;
      uint32 w01 = fs - w11;
      uint32 w00 = 16 - fs - ft + w11;

      uint32 v0 = js + jt * params.m_Width;

      #define FIND_TEXEL(tidx, bidx)                            \
      uint32 p##bidx = 0;                                       \
      do {                                                      \
        if(w##bidx > 0) {                                       \
          assert((tidx) < (params.m_Width * params.m_Height));  \
          p##bidx = unquantized[plane][(tidx)];                 \
        }                                                       \
      }                                                         \
      while(0)

      FIND_TEXEL(v0, 00);
      FIND_TEXEL(v0 + 1, 01);
      FIND_TEXEL(v0 + params.m_Width, 10);
      FIND_TEXEL(v0 + params.m_Width + 1, 11);

      #undef FIND_TEXEL

      out[plane][t*blockWidth + s] = (p00*w00 + p01*w01 + p10*w10 + p11*w11 + 8) >> 4;
    }
  }

  // Section C.2.14
  void ComputeEndpoints(FasTC::Pixel &ep1, FasTC::Pixel &ep2,
                        const uint32* colorValuesPtr, uint32 colorEndpointMode) {
    const uint32 *colorValues = colorValuesPtr;
    #define READ_UINT_VALUES(N)                 \
      uint32 v[N];                              \
      for(uint32 i = 0; i < N; i++) {           \
        v[i] = *(colorValues++);                \
      }

    #define READ_INT_VALUES(N)                          \
      int32 v[N];                                       \
      for(uint32 i = 0; i < N; i++) {                   \
        v[i] = static_cast<int32>(*(colorValues++));    \
      }
    
    switch(colorEndpointMode) {
      case 0: {
        READ_UINT_VALUES(2)
        ep1 = FasTC::Pixel(0xFF, v[0], v[0], v[0]);
        ep2 = FasTC::Pixel(0xFF, v[1], v[1], v[1]);
      }
      break;

      case 1: {
        READ_UINT_VALUES(2)
        uint32 L0 = (v[0] >> 2) | (v[1] & 0xC0);
        uint32 L1 = std::max(L0 + (v[1] & 0x3F), 0xFFU);
        ep1 = FasTC::Pixel(0xFF, L0, L0, L0);
        ep2 = FasTC::Pixel(0xFF, L1, L1, L1);
      }
      break;

      case 4: {
        READ_UINT_VALUES(4)
        ep1 = FasTC::Pixel(v[2], v[0], v[0], v[0]);
        ep2 = FasTC::Pixel(v[3], v[1], v[1], v[1]);
      }
      break;

      case 5: {
        READ_INT_VALUES(4)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        ep1 = FasTC::Pixel(v[2], v[0], v[0], v[0]);
        ep2 = FasTC::Pixel(v[2]+v[3], v[0]+v[1], v[0]+v[1], v[0]+v[1]);
        ep1.ClampByte();
        ep2.ClampByte();
      }
      break;

      case 6: {
        READ_UINT_VALUES(4)
        ep1 = FasTC::Pixel(0xFF, v[0]*v[3] >> 8, v[1]*v[3] >> 8, v[2]*v[3] >> 8);
        ep2 = FasTC::Pixel(0xFF, v[0], v[1], v[2]);
      }
      break;

      case 8: {
        READ_UINT_VALUES(6)
        if(v[1]+v[3]+v[5] >= v[0]+v[2]+v[4]) {
          ep1 = FasTC::Pixel(0xFF, v[0], v[2], v[4]);
          ep2 = FasTC::Pixel(0xFF, v[1], v[3], v[5]);
        } else {
          ep1 = BlueContract(0xFF, v[1], v[3], v[5]);
          ep2 = BlueContract(0xFF, v[0], v[2], v[4]);
        }
      }
      break;

      case 9: {
        READ_INT_VALUES(6)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        BitTransferSigned(v[5], v[4]);
        if(v[1]+v[3]+v[5] >= 0) {
          ep1 = FasTC::Pixel(0xFF, v[0], v[2], v[4]);
          ep2 = FasTC::Pixel(0xFF, v[0]+v[1], v[2]+v[3], v[4]+v[5]);
        } else {
          ep1 = BlueContract(0xFF, v[0]+v[1], v[2]+v[3], v[4]+v[5]);
          ep2 = BlueContract(0xFF, v[0], v[2], v[4]);
        }
        ep1.ClampByte();
        ep2.ClampByte();
      }
      break;

      case 10: {
        READ_UINT_VALUES(6)
        ep1 = FasTC::Pixel(v[4], v[0]*v[3] >> 8, v[1]*v[3] >> 8, v[2]*v[3] >> 8);
        ep2 = FasTC::Pixel(v[5], v[0], v[1], v[2]);
      }
      break;

      case 12: {
        READ_UINT_VALUES(8)
        if(v[1]+v[3]+v[5] >= v[0]+v[2]+v[4]) {
          ep1 = FasTC::Pixel(v[6], v[0], v[2], v[4]);
          ep2 = FasTC::Pixel(v[7], v[1], v[3], v[5]);
        } else {
          ep1 = BlueContract(v[7], v[1], v[3], v[5]);
          ep2 = BlueContract(v[6], v[0], v[2], v[4]);
        }
      }
      break;

      case 13: {
        READ_INT_VALUES(8)
        BitTransferSigned(v[1], v[0]);
        BitTransferSigned(v[3], v[2]);
        BitTransferSigned(v[5], v[4]);
        BitTransferSigned(v[7], v[6]);
        if(v[1]+v[3]+v[5] >= 0) {
          ep1 = FasTC::Pixel(v[6], v[0], v[2], v[4]);
          ep2 = FasTC::Pixel(v[7]+v[6], v[0]+v[1], v[2]+v[3], v[4]+v[5]);
        } else {
          ep1 = BlueContract(v[6]+v[7], v[0]+v[1], v[2]+v[3], v[4]+v[5]);
          ep2 = BlueContract(v[6], v[0], v[2], v[4]);
        }
        ep1.ClampByte();
        ep2.ClampByte();
      }
      break;

      default:
        assert(!"Unsupported color endpoint mode (is it HDR?)");
        break;
    }

    #undef READ_UINT_VALUES
    #undef READ_INT_VALUES
  }

  void DecompressBlock(const uint8 inBuf[16],
                       const uint32 blockWidth, const uint32 blockHeight,
                       uint32 *outBuf) {
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
    memcpy(texelWeightData, inBuf, sizeof(texelWeightData));
    
    // Reverse everything
    for(uint32 i = 0; i < 8; i++) {
      // Taken from http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
      #define REVERSE_BYTE(b) (((b) * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32
      unsigned char a = REVERSE_BYTE(texelWeightData[i]);
      unsigned char b = REVERSE_BYTE(texelWeightData[15 - i]);
      #undef REVERSE_BYTE

      texelWeightData[i] = b;
      texelWeightData[15-i] = a;
    }

    // Decode both color data and texel weight data
    uint32 colorValues[32]; // Four values, two endpoints, four maximum paritions
    DecodeColorValues(colorValues, colorEndpointData, colorEndpointMode,
                      nPartitions, colorDataBits);

    std::vector<IntegerEncodedValue> texelWeightValues;
    FasTC::BitStreamReadOnly weightStream (texelWeightData);

    IntegerEncodedValue::
      DecodeIntegerSequence(texelWeightValues, weightStream,
                            weightParams.m_MaxWeight,
                            weightParams.GetNumWeightValues());

    FasTC::Pixel endpoints[4][2];
    for(uint32 i = 0; i < nPartitions; i++) {
      ComputeEndpoints(endpoints[i][0], endpoints[i][1],
                       colorValues, colorEndpointMode[i]);
    }

    // Blocks can be at most 12x12, so we can have as many as 144 weights
    uint32 weights[2][144];
    UnquantizeTexelWeights(weights, texelWeightValues, weightParams, blockWidth, blockHeight);

    // Now that we have endpoints and weights, we can interpolate and generate
    // the proper decoding...
    for(uint32 j = 0; j < blockHeight; j++)
    for(uint32 i = 0; i < blockWidth; i++) {
      uint32 partition = Select2DPartition(
        partitionIndex, i, j, nPartitions, (blockHeight * blockWidth) < 32
      );
      assert(partition < nPartitions);

      FasTC::Pixel p;
      for(uint32 c = 0; c < 4; c++) {
        uint32 C0 = endpoints[partition][0].Component(c);
        C0 = FasTC::Replicate(C0, 8, 16);
        uint32 C1 = endpoints[partition][1].Component(c);
        C1 = FasTC::Replicate(C1, 8, 16);

        uint32 plane = 0;
        if(weightParams.m_bDualPlane && (((planeIdx + 1) & 3) == c)) {
          plane = 1;
        }

        uint32 weight = weights[plane][j * blockWidth + i];
        uint32 C = (C0 * (64 - weight) + C1 * weight + 32) / 64;
        p.Component(c) = C >> 8;
      }

      outBuf[j * blockWidth + i] = p.Pack();
    }
  }

  void Decompress(const FasTC::DecompressionJob &dcj) {
    uint32 blockWidth = GetBlockWidth(dcj.Format());
    uint32 blockHeight = GetBlockHeight(dcj.Format());
    uint32 blockIdx = 0;
    for(uint32 j = 0; j < dcj.Height(); j+=blockHeight) {
      for(uint32 i = 0; i < dcj.Width(); i+=blockWidth) {

        const uint8 *blockPtr = dcj.InBuf() + blockIdx*16;

        // Blocks can be at most 12x12
        uint32 uncompData[144];
        DecompressBlock(blockPtr, blockWidth, blockHeight, uncompData);

        uint8 *outRow = dcj.OutBuf() + (j*dcj.Width() + i)*4;
        for(uint32 jj = 0; jj < blockHeight; jj++) {
          memcpy(outRow + jj*dcj.Width()*4, uncompData + jj*blockWidth, blockWidth*4);
        }

        blockIdx++;
      }
    }
  }

}  // namespace ASTCC
