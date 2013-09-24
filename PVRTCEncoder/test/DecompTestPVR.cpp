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

#include "gtest/gtest.h"

// This is the PVR library header
#include "PVRTextureUtilities.h"

// This is our library header
#include "PVRTCCompressor.h"

#include "TestUtils.h"

// #define OUTPUT_DEBUG_IMAGE

#ifdef OUTPUT_DEBUG_IMAGE
#  include "Core/include/Image.h"
#  include "IO/include/ImageFile.h"
#endif  // OUTPUT_DEBUG_IMAGE

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

    DecompressionJob dcj(data, reinterpret_cast<uint8 *>(outPixels), w, h);
#ifdef OUTPUT_DEBUG_IMAGE
    PVRTCC::Decompress(dcj, twobpp, PVRTCC::eWrapMode_Wrap, true);
#else
    PVRTCC::Decompress(dcj, twobpp, PVRTCC::eWrapMode_Wrap);
#endif

    bool result = pvrtexture::Transcode(pvrTex,
                                        pvrtexture::PVRStandard8PixelType,
                                        ePVRTVarTypeUnsignedByte,
                                        ePVRTCSpacelRGB);
    EXPECT_TRUE(result);

    uint32 *libPixels = static_cast<uint32 *>(pvrTex.getDataPtr());

    for(int i = 0; i < w*h; i++) {
      EXPECT_EQ(PixelPrinter(libPixels[i]), PixelPrinter(outPixels[i]));
    }

#ifdef OUTPUT_DEBUG_IMAGE
    char dbgfname[256];
    snprintf(dbgfname, sizeof(dbgfname), "Debug%s.png", filename);
    ::ImageFile imgFile(dbgfname, eFileFormat_PNG, ::Image(w, h, outPixels));
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
