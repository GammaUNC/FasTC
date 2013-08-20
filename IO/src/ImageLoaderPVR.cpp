/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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

#include "ImageLoaderPVR.h"

#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include "TexCompTypes.h"

static void ReportError(const char *msg) {
  fprintf(stderr, "ERROR: ImageLoaderPVR -- %s\n", msg);
}

ImageLoaderPVR::ImageLoaderPVR(const unsigned char *rawData) 
  : ImageLoader(rawData)
{
}

ImageLoaderPVR::~ImageLoaderPVR() {
}

bool ImageLoaderPVR::ReadData() {
  const uint8 *d = m_RawData;
  if((d[0] == 'P' && d[1] == 'V' && d[2] == 'R' && d[3] == '\03') ||
     (d[0] == '\03' && d[1] == 'R' && d[2] == 'V' && d[3] == 'P')) {
    return ReadPVR3();
  }
  else {
    ReportError("Unsupported PVR version");
    return false;
  }
}

bool ImageLoaderPVR::ReadPVR1() {
  return false;
}

bool ImageLoaderPVR::ReadPVR2() {
  return false;
}

struct PVR3TexHeader {
 private:
  uint32 m_Version;
  uint32 m_Flags;
  uint64 m_PixelFormat;
  uint32 m_ColorSpace;
  uint32 m_ChannelType;
  uint32 m_Height;
  uint32 m_Width;
  uint32 m_Depth;
  uint32 m_NumSurfaces;
  uint32 m_NumFaces;
  uint32 m_MipMapCount;
  uint32 m_MetadataSize;

  static void FlipBytes(uint8 *bytes, int nBytes) {
    for (int i = 0; i < (nBytes >> 1); i++) {
      std::swap(bytes[i], bytes[nBytes - i - 1]);
    }
  }

  static uint32 FlipInt32(uint32 in) {
    uint8 *inPtr = (uint8 *)(&in);
    FlipBytes(inPtr, 4);
    return in;
  }

  static uint64 FlipInt64(uint64 in) {
    uint8 *inPtr = (uint8 *)(&in);
    FlipBytes(inPtr, 8);
    return in;
  }

  bool ShouldFlip() const {
    const uint8 *versionPtr = (const uint8 *)(&m_Version);
    if(versionPtr[0] == 'P' &&
       versionPtr[1] == 'V' &&
       versionPtr[2] == 'R' &&
       versionPtr[3] == '\03') {
      return false;
    }
    return true;
  }

 public:

#define CONSTRUCT_GETTER(name) \
  uint32 Get##name () const {  \
    return ShouldFlip()? FlipInt32(m_##name) : m_##name; \
  }

  CONSTRUCT_GETTER(Flags)
  CONSTRUCT_GETTER(ColorSpace)
  CONSTRUCT_GETTER(ChannelType)
  CONSTRUCT_GETTER(Height)
  CONSTRUCT_GETTER(Width)
  CONSTRUCT_GETTER(Depth)
  CONSTRUCT_GETTER(NumSurfaces)
  CONSTRUCT_GETTER(NumFaces)
  CONSTRUCT_GETTER(MipMapCount)
  CONSTRUCT_GETTER(MetadataSize)

#undef CONSTRUCT_GETTER

  uint64 GetPixelFormat() const {
    return ShouldFlip()? FlipInt64(m_PixelFormat) : m_PixelFormat;
  }

  enum EFlag {
    eFlag_Premultiplied = 0x02
  };

  enum EPixelFormat {
    ePixelFormat_PVRTC_2BPP_RGB = 0,
    ePixelFormat_PVRTC_2BPP_RGBA = 1,
    ePixelFormat_PVRTC_4BPP_RGB = 2,
    ePixelFormat_PVRTC_4BPP_RGBA = 3,
    ePixelFormat_PVRTC2_2BPP = 4,
    ePixelFormat_PVRTC2_4BPP = 5,
    ePixelFormat_ETC1 = 6,
    ePixelFormat_DXT1 = 7,
    ePixelFormat_DXT2 = 8,
    ePixelFormat_DXT3 = 9,
    ePixelFormat_DXT4 = 10,
    ePixelFormat_DXT5 = 11,
    ePixelFormat_BC1 = ePixelFormat_DXT1,
    ePixelFormat_BC2 = ePixelFormat_DXT3,
    ePixelFormat_BC3 = ePixelFormat_DXT5,
    ePixelFormat_BC4 = 12,
    ePixelFormat_BC5 = 13,
    ePixelFormat_BC6 = 14,
    ePixelFormat_BC7 = 15,
    ePixelFormat_UYVY = 16,
    ePixelFormat_YUY2 = 17,
    ePixelFormat_BW1BPP = 18,
    ePixelFormat_R9G9B9E5_SE = 19,
    ePixelFormat_RGBG8888 = 20,
    ePixelFormat_GRGB8888 = 21,
    ePixelFormat_ETC2_RGB = 22,
    ePixelFormat_ETC2_RGBA = 23,
    ePixelFormat_ETC2_RGB_A1 = 24,
    ePixelFormat_EAC_R11_UNSIGNED = 25,
    ePixelFormat_EAC_R11_SIGNED = 26,
    ePixelFormat_EAC_RG11_UNSIGNED = 27,
    ePixelFormat_EAC_RG11_SIGNED = 28
  };

  enum EColorSpace {
    eColorSpace_Linear,
    eColorSpace_sRGB
  };

  enum EChannelType {
    eChannelType_UnsignedByteNormalized = 0,
    eChannelType_SignedByteNormalized,
    eChannelType_UnsignedByte,
    eChannelType_SignedByte,
    eChannelType_UnsignedShortNormalized,
    eChannelType_SignedShortNormalized,
    eChannelType_UnsignedShort,
    eChannelType_SignedShort,
    eChannelType_UnsignedIntegerNormalized,
    eChannelType_SignedIntegerNormalized,
    eChannelType_UnsignedInteger,
    eChannelType_SignedInteger,
    eChannelType_Float,
  };
};

bool ImageLoaderPVR::ReadPVR3() {
  return false;
}
