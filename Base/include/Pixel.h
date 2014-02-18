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

#ifndef BASE_INCLUDE_PIXEL_H_
#define BASE_INCLUDE_PIXEL_H_

#include "TexCompTypes.h"
#include "Vector4.h"

namespace FasTC {

class Pixel : public Vector4<uint16> {
 protected:
  typedef uint16 ChannelType;
  typedef Vector4<ChannelType> VectorType;
  uint8 m_BitDepth[4];

 public:
  Pixel() : VectorType(0, 0, 0, 0) {
    for(int i = 0; i < 4; i++)
      m_BitDepth[i] = 8;
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
