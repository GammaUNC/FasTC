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

// This is our library header
#include "ASTCCompressor.h"

#include "ImageFile.h"
#include "Image.h"

class ImageTester {
 private:
  static void GenerateTestFilenames(const char *suffix, char *outASTC, char *outPNG) {
    const char *basename = "mandrill";
    sprintf(outASTC, "%s_%s.astc", basename, suffix);
    sprintf(outPNG, "%s_decompressed_%s.png", basename, suffix);
  }

 public:
  explicit ImageTester(const char *suffix) {
    char astcFilename[256];
    char pngFilename[256];

    GenerateTestFilenames(suffix, astcFilename, pngFilename);

    ImageFile astc (astcFilename);
    bool success = astc.Load();
    EXPECT_TRUE(success);
    if (!success) {
      return;
    }

    ImageFile png (pngFilename);
    success = png.Load();
    EXPECT_TRUE(success);
    if (!success) {
      return;
    }

    double PSNR = astc.GetImage()->ComputePSNR(png.GetImage());
    EXPECT_GT(PSNR, 60.0);
  }
};

// 4x4 12x12 8x8 6x5 10x8
TEST(Decompressor, Decompress4x4) {
  ImageTester("4x4");
}

TEST(Decompressor, Decompress12x12) {
  ImageTester("12x12");
}

TEST(Decompressor, Decompress8x8) {
  ImageTester("8x8");
}

TEST(Decompressor, Decompress6x5) {
  ImageTester("6x5");
}

TEST(Decompressor, Decompress10x8) {
  ImageTester("10x8");
}
