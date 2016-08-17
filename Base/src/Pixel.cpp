// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

#include "FasTC/Pixel.h"

#include <cstring>
#include <cassert>
#include <algorithm>

#include "FasTC/Bits.h"

template<typename T>
static inline T Clamp(const T &v, const T &_min, const T &_max) {
  return std::max(_min, std::min(v, _max));
}

namespace FasTC {

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
      ChannelType &channel = Component(i);
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

  void Pixel::ToBits(uint8 *bits, uint32 numBytes, uint32 bitOffset) const {
#ifndef NDEBUG
    uint32 bitDepthSum = bitOffset;
    for(int i = 0; i < 4; i++) {
      bitDepthSum += m_BitDepth[i];
    }
    assert((bitDepthSum / 8) < numBytes);
#endif

    uint8 byteIdx = 0;
    while(bitOffset > 8) {
      byteIdx++;
      bitOffset -= 8;
    }

    uint8 bitIdx = bitOffset;
    for(int i = 3; i >= 0; i--) {
      ChannelType val = Component(i);
      uint8 depth = m_BitDepth[i];

      if(depth + bitIdx > 8) {
        uint8 nextBitIdx = depth - (8 - bitIdx);
        uint16 v = static_cast<uint16>(val);
        bits[byteIdx++] |= (v << bitIdx) & 0xFF;
        bitIdx = nextBitIdx;
        bits[byteIdx] = (v >> (depth - bitIdx)) & 0xFF;
      } else {
        bits[byteIdx] |= (val << bitIdx) & 0xFF;
        bitIdx += depth;
      }

      if(bitIdx == 8) {
        bitIdx = 0;
        byteIdx++;
      }
    }
  }

  Pixel::ChannelType Pixel::ChangeBitDepth(Pixel::ChannelType val, uint8 oldDepth, uint8 newDepth) {
    assert(newDepth <= 8);
    assert(oldDepth <= 8);

    if(oldDepth == newDepth) {
      // Do nothing
      return val;
    } else if(oldDepth == 0 && newDepth != 0) {
      return (1 << newDepth) - 1;
    } else if(newDepth > oldDepth) {
      return Replicate(val, oldDepth, newDepth);
    } else {
      // oldDepth > newDepth
      if(newDepth == 0) {
        return 0xFF;
      } else {
        uint8 bitsWasted = oldDepth - newDepth;
        uint16 v = static_cast<uint16>(val);
        v = (v + (1 << (bitsWasted - 1))) >> bitsWasted;
        v = ::std::min<uint16>(::std::max<uint16>(0, v), (1 << newDepth) - 1);
        return static_cast<uint8>(v);
      }
    }

    assert(!"We shouldn't get here.");
    return 0;
  }

  void Pixel::ChangeBitDepth(const uint8 (&depth)[4]) {
    for(uint32 i = 0; i < 4; i++) {
      Component(i) = ChangeBitDepth(Component(i), m_BitDepth[i], depth[i]);
      m_BitDepth[i] = depth[i];
    }
  }

  float Pixel::ToIntensity() const {
    // First convert the pixel values to floats using premultiplied alpha...
    double a = ConvertChannelToFloat(A(), m_BitDepth[0]);
    double r = a * ConvertChannelToFloat(R(), m_BitDepth[1]);
    double g = a * ConvertChannelToFloat(G(), m_BitDepth[2]);
    double b = a * ConvertChannelToFloat(B(), m_BitDepth[3]);
    return static_cast<float>(r * 0.2126 + g * 0.7152 + b * 0.0722);
  }

  uint32 Pixel::Pack() const {
    Pixel eightBit(*this);
    const uint8 eightBitDepth[4] = { 8, 8, 8, 8 };
    eightBit.ChangeBitDepth(eightBitDepth);

    uint32 r = 0;
    r |= eightBit.A();
    r <<= 8;
    r |= eightBit.B();
    r <<= 8;
    r |= eightBit.G();
    r <<= 8;
    r |= eightBit.R();
    return r;
  }

  void Pixel::Unpack(uint32 rgba) {
    A() = ChangeBitDepth((rgba >> 24) & 0xFF, 8, m_BitDepth[0]);
    R() = ChangeBitDepth(rgba & 0xFF, 8, m_BitDepth[1]);
    G() = ChangeBitDepth((rgba >> 8) & 0xFF, 8, m_BitDepth[2]);
    B() = ChangeBitDepth((rgba >> 16) & 0xFF, 8, m_BitDepth[3]);
  }

  void Pixel::Shuffle(uint8 shuffleMask) {
    Pixel thisPixel(*this);
    uint8 a = shuffleMask & 3;
    uint8 b = (shuffleMask >> 2) & 3;
    uint8 c = (shuffleMask >> 4) & 3;
    uint8 d = (shuffleMask >> 6) & 3;

    Pixel tmp;
    tmp[0] = thisPixel[a];
    tmp.m_BitDepth[0] = thisPixel.m_BitDepth[a];

    tmp[1] = thisPixel[b];
    tmp.m_BitDepth[1] = thisPixel.m_BitDepth[b];

    tmp[2] = thisPixel[c];
    tmp.m_BitDepth[2] = thisPixel.m_BitDepth[c];

    tmp[3] = thisPixel[d];
    tmp.m_BitDepth[3] = thisPixel.m_BitDepth[d];

    *this = tmp;
  }

  bool Pixel::operator==(const Pixel &other) const {
    uint8 depths[4];
    other.GetBitDepth(depths);

    bool ok = true;
    for(int i = 0; i < 4; i++) {
      ok = ok && m_BitDepth[i] == depths[i];

      uint8 mask = (1 << depths[i]) - 1;
      const ChannelType c = other.Component(i) & mask;
      ok = ok && (c == (Component(i) & mask));
    }
    return ok;
  }

  void YCoCgPixel::ToYCoCg() {
    int16 Y = ((R() + (G() << 1) + B()) + 2) >> 2;
    int16 Co = (R() - B() + 1) >> 1;
    int16 Cg = ((-R() + (G() << 1) - B()) + 2) >> 2;

    this->Y() = Clamp<int16>(Y, 0, 255);
    this->Co() = Clamp<int16>(Co + 128, 0, 255);
    this->Cg() = Clamp<int16>(Cg + 128, 0, 255);
  }

  Pixel YCoCgPixel::ToRGBA() const {
    int16 Co = this->Co() - 128;
    int16 Cg = this->Cg() - 128;

    int16 R = Y() + (Co - Cg);
    int16 G = Y() + Cg;
    int16 B = Y() - (Co + Cg);

    Pixel p;
    p.R() = Clamp<int16>(R, 0, 255);
    p.G() = Clamp<int16>(G, 0, 255);
    p.B() = Clamp<int16>(B, 0, 255);
    p.A() = A();
    return p;
  }

}  // namespace FasTC
