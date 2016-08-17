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

#ifndef BASE_INCLUDE_PIXEL_H_
#define BASE_INCLUDE_PIXEL_H_

#include "TexCompTypes.h"
#include "Vector4.h"

namespace FasTC {

class Pixel : public Vector4<int16> {
 protected:
  typedef int16 ChannelType;
  typedef Vector4<ChannelType> VectorType;
  uint8 m_BitDepth[4];

 public:
  Pixel() : VectorType(0, 0, 0, 0) {
    for(int i = 0; i < 4; i++)
      m_BitDepth[i] = 8;
  }

  Pixel(ChannelType a, ChannelType r, ChannelType g, ChannelType b, unsigned bitDepth = 8)
    : VectorType(a, r, g, b)
  {
    for(int i = 0; i < 4; i++)
      m_BitDepth[i] = bitDepth;
  }

  explicit Pixel(uint32 rgba) : VectorType() {
    for(int i = 0; i < 4; i++)
      m_BitDepth[i] = 8;
    Unpack(rgba);
  }

  Pixel(const uint8 *bits,
        const uint8 channelDepth[4] = static_cast<uint8 *>(0),
        uint8 bitOffset = 0) : VectorType() {
    FromBits(bits, channelDepth, bitOffset);
  }

  // Reads a pixel from memory given the bit depth. If NULL then
  // it is assumed to be 8 bit RGBA. The bit offset is the offset
  // from the least significant bit from which we start reading
  // the pixel values.
  void FromBits(const uint8 *bits,
                const uint8 channelDepth[4] = static_cast<uint8 *>(0),
                uint8 bitOffset = 0);

  // This function is the converse of FromBits. It will pack a pixel
  // into a specified buffer based on the bit depth of the pixel. The
  // bitOffset determines at which bit to start from. The bits are written
  // starting from the LSB of bits[0]. numBytes is a sanity check and isn't
  // used in release mode.
  void ToBits(uint8 *bits, uint32 numBytes, uint32 bitOffset = 0) const;

  // Changes the depth of each pixel. This scales the values to
  // the appropriate bit depth by either truncating the least
  // significant bits when going from larger to smaller bit depth
  // or by repeating the most significant bits when going from
  // smaller to larger bit depths.
  void ChangeBitDepth(const uint8 (&newDepth)[4]);

  template<typename IntType>
  static float ConvertChannelToFloat(IntType channel, uint8 bitDepth) {
    float denominator = static_cast<float>((1 << bitDepth) - 1);
    return static_cast<float>(channel) / denominator;
  }

  // Returns the intensity of the pixel. Computed using the following 
  // formula:
  // a*r*0.21f + a*g*0.71f + a*b*0.07f;
  float ToIntensity() const;

  // Changes the bit depth of a single component. See the comment
  // above for how we do this.
  static ChannelType ChangeBitDepth(ChannelType val, uint8 oldDepth, uint8 newDepth);

  const ChannelType &A() const { return X(); }
  ChannelType &A() { return X(); }
  const ChannelType &R() const { return Y(); }
  ChannelType &R() { return Y(); }
  const ChannelType &G() const { return Z(); }
  ChannelType &G() { return Z(); }
  const ChannelType &B() const { return W(); }
  ChannelType &B() { return W(); }
  const ChannelType &Component(uint32 idx) const { return vec[idx]; }
  ChannelType &Component(uint32 idx) { return vec[idx]; }

  void GetBitDepth(uint8 (&outDepth)[4]) const {
    for(int i = 0; i < 4; i++) {
      outDepth[i] = m_BitDepth[i];
    }
  }

  // Take all of the components, transform them to their 8-bit variants,
  // and then pack each channel into an R8G8B8A8 32-bit integer. We assume
  // that the architecture is little-endian, so the alpha channel will end
  // up in the most-significant byte.
  uint32 Pack() const;
  void Unpack(uint32 rgba);

  // Shuffles the pixel values around so that they change their ordering based
  // on the passed mask. The values are chosen such that each two bits from the
  // least significant bit define a value from 0-3. From LSB to MSB, the values
  // are labelled a, b, c, d. From these labels, we store:
  // m_Pixels[0] = m_Pixels[a]
  // m_Pixels[1] = m_Pixels[b]
  // m_Pixels[2] = m_Pixels[c]
  // m_Pixels[3] = m_Pixels[d]
  // hence, 0xE4 (11 10 01 00) represents a no-op.
  void Shuffle(uint8 shuffleMask = 0xE4);

  // Tests for equality by comparing the values and the bit depths.
  bool operator==(const Pixel &) const;

  // Clamps the pixel to the range [0,255]
  void ClampByte() {
    for(uint32 i = 0; i < 4; i++) {
      vec[i] = (vec[i] < 0)? 0 : ((vec[i] > 255)? 255 : vec[i]);
    }
  }

  void MakeOpaque() { A() = 255; }
};
REGISTER_VECTOR_TYPE(Pixel);

class YCoCgPixel : public Pixel {
 private:
  void ToYCoCg();

 public:
  YCoCgPixel() : Pixel() { }
  explicit YCoCgPixel(uint32 rgba) : Pixel(rgba) { ToYCoCg(); }
  explicit YCoCgPixel(const Pixel &p) : Pixel(p) { ToYCoCg(); }

  Pixel ToRGBA() const;

  float ToIntensity() const { return ConvertChannelToFloat(R(), 8); }
  uint32 Pack() const { return ToRGBA().Pack(); }
  void Unpack(uint32 rgba) { Pixel::Unpack(rgba); ToYCoCg(); }

  const ChannelType &Co() const { return Z(); }
  ChannelType &Co() { return Z(); }
  const ChannelType &Cg() const { return W(); }
  ChannelType &Cg() { return W(); }  
};
REGISTER_VECTOR_TYPE(YCoCgPixel);

}  // namespace FasTC

#endif  // BASE_INCLUDE_PIXEL_H_
