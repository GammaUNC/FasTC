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

#include "gtest/gtest.h"

// This is the PVR library header
#include "PVRTextureUtilities.h"

// This is our library header
#include "FasTC/PVRTCCompressor.h"

#include "TestUtils.h"

#ifdef DEBUG_PVRTC_DECODER
#  include "Image.h"
#  include "ImageFile.h"
#endif  // DEBUG_PVRTC_DECODER

class ImageTester {
 public:
  explicit ImageTester(const char *filename, bool twobpp) {
    pvrtexture::CPVRTexture pvrTex(filename);

    const uint8 *data = static_cast<const uint8 *>(pvrTex.getDataPtr());
    assert(data);

    const pvrtexture::CPVRTextureHeader &hdr = pvrTex.getHeader();
    const uint32 w = hdr.getWidth();
    const uint32 h = hdr.getHeight();

    uint32 *outPixels = new uint32[w * h];

    FasTC::ECompressionFormat fmt = FasTC::eCompressionFormat_PVRTC4;
    if(twobpp) {
      fmt = FasTC::eCompressionFormat_PVRTC2;
    }

    uint8 *outBuf = reinterpret_cast<uint8 *>(outPixels);
    FasTC::DecompressionJob dcj(fmt, data, outBuf, w, h);
#ifdef DEBUG_PVRTC_DECODER
    PVRTCC::Decompress(dcj, PVRTCC::eWrapMode_Wrap, true);
#else
    PVRTCC::Decompress(dcj, PVRTCC::eWrapMode_Wrap);
#endif

    bool result = pvrtexture::Transcode(pvrTex,
                                        pvrtexture::PVRStandard8PixelType,
                                        ePVRTVarTypeUnsignedByte,
                                        ePVRTCSpacelRGB);
    EXPECT_TRUE(result);

    uint32 *libPixels = static_cast<uint32 *>(pvrTex.getDataPtr());

    for(uint32 i = 0; i < w*h; i++) {
      EXPECT_EQ(PixelPrinter(libPixels[i]), PixelPrinter(outPixels[i]));
    }

#ifdef DEBUG_PVRTC_DECODER
    char dbgfname[256];
    snprintf(dbgfname, sizeof(dbgfname), "Debug%s.png", filename);
    ImageFile imgFile(dbgfname, eFileFormat_PNG, FasTC::Image<>(w, h, outPixels));
    imgFile.Write();
#endif  // OUTPUT_DEBUG_IMAGE

    delete outPixels;
  }
};

TEST(Decompressor, DecompressWhite2BPP) {
  ImageTester("2bpp-white.pvr", true);
}

TEST(Decompressor, DecompressGray2BPP) {
  ImageTester("2bpp-gray.pvr", true);
}

TEST(Decompressor, DecompressGradient2BPP) {
  ImageTester("2bpp-gradient.pvr", true);
}

TEST(Decompressor, DecompressTransparent2BPP) {
  ImageTester("2bpp-transparent.pvr", true);
}

TEST(Decompressor, DecompressTransGradient2BPP) {
  ImageTester("2bpp-trans-gradient.pvr", true);
}

TEST(Decompressor, DecompressWhite4BPP) {
  ImageTester("4bpp-white.pvr", false);
}

TEST(Decompressor, DecompressGray4BPP) {
  ImageTester("4bpp-gray.pvr", false);
}

TEST(Decompressor, DecompressGradient4BPP) {
  ImageTester("4bpp-gradient.pvr", false);
}

TEST(Decompressor, DecompressTransparent4BPP) {
  ImageTester("4bpp-transparent.pvr", false);
}

TEST(Decompressor, DecompressTransGradient4BPP) {
  ImageTester("4bpp-trans-gradient.pvr", false);
}
