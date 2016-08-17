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

// This is our library header
#include "FasTC/ASTCCompressor.h"

#include "FasTC/ImageFile.h"
#include "FasTC/Image.h"

#include <string>

class ImageTester {
 private:
  static std::string GenerateTestFilename(const std::string &basename,
                                          const std::string &suffix,
                                          const std::string &ext) {
    return basename + suffix + std::string(".") + ext;
  }

 public:
  explicit ImageTester(const char *suffix) {
    std::string astcFilename =
      GenerateTestFilename("mandrill_", std::string(suffix), "astc");
    std::string pngFilename =
      GenerateTestFilename("mandrill_decompressed_", std::string(suffix), "png");

    ImageFile astc (astcFilename.c_str());
    bool success = astc.Load();
    EXPECT_TRUE(success);
    if (!success) {
      return;
    }

    ImageFile png (pngFilename.c_str());
    success = png.Load();
    EXPECT_TRUE(success);
    if (!success) {
      return;
    }

    double PSNR = astc.GetImage()->ComputePSNR(png.GetImage());
    EXPECT_GT(PSNR, 60.0);
  }
};

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
