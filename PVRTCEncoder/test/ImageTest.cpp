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
#include "PVRTCImage.h"
#include "FasTC/Pixel.h"
#include "TestUtils.h"

#include <cstdlib>

TEST(Image, BilinearUpscale) {
  FasTC::Pixel pxs[16];
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i*2;
      pxs[j*4 + i].G() = j*2;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  img.BilinearUpscale(1, 1, PVRTCC::eWrapMode_Clamp);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(8));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(8));

  for(uint32 i = 0; i < img.GetWidth(); i++) {
    for(uint32 j = 0; j < img.GetHeight(); j++) {
      if(i == 0) {
        EXPECT_EQ(static_cast<uint32>(img(i, j).R()), i);
      } else {
        EXPECT_EQ(static_cast<uint32>(img(i, j).R()), i-1);
      }

      if(j == 0) {
        EXPECT_EQ(static_cast<uint32>(img(i, j).G()), j);
      } else {
        EXPECT_EQ(static_cast<uint32>(img(i, j).G()), j-1);
      }
    }
  }
}

TEST(Image, BilinearUpscaleMaintainsPixels) {

  srand(0xabd1ca7e);

  const uint32 w = 4;
  const uint32 h = 4;

  FasTC::Pixel pxs[16];
  for(uint32 i = 0; i < w; i++) {
    for(uint32 j = 0; j < h; j++) {
      pxs[j*w + i].R() = rand() % 256;
      pxs[j*w + i].G() = rand() % 256;
      pxs[j*w + i].B() = rand() % 256;
      pxs[j*w + i].A() = rand() % 256;
    }
  }

  PVRTCC::Image img(w, h, pxs);
  img.BilinearUpscale(2, 2, PVRTCC::eWrapMode_Clamp);
  EXPECT_EQ(img.GetWidth(), w << 2);
  EXPECT_EQ(img.GetHeight(), h << 2);

  for(uint32 i = 2; i < img.GetWidth(); i+=4) {
    for(uint32 j = 2; j < img.GetHeight(); j+=4) {
      FasTC::Pixel p = img(i, j);
      uint32 idx = ((j - 2) / 4) * w + ((i-2)/4);
      EXPECT_EQ(PixelPrinter(p.Pack()), PixelPrinter(pxs[idx].Pack()));
    }
  }
}


TEST(Image, NonuniformBilinearUpscale) {

  const uint32 kWidth = 4;
  const uint32 kHeight = 8;

  FasTC::Pixel pxs[kWidth * kHeight];
  for(uint32 i = 0; i < kWidth; i++) {
    for(uint32 j = 0; j < kHeight; j++) {
      pxs[j*kWidth + i].R() = i*4;
      pxs[j*kWidth + i].G() = j*2;
    }
  }

  PVRTCC::Image img(kWidth, kHeight, pxs);
  img.BilinearUpscale(2, 1, PVRTCC::eWrapMode_Clamp);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(kWidth << 2));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(kHeight << 1));

  for(uint32 i = 0; i < img.GetWidth(); i++) {
    for(uint32 j = 0; j < img.GetHeight(); j++) {
      if(i <= 2) {
        EXPECT_EQ(img(i, j).R(), 0);
      } else if(i == 15) {
        EXPECT_EQ(img(i, j).R(), 12);
      } else {
        EXPECT_EQ(static_cast<uint32>(img(i, j).R()), i-2);
      }

      if(j == 0) {
        EXPECT_EQ(img(i, j).G(), 0);
      } else {
        EXPECT_EQ(static_cast<uint32>(img(i, j).G()), j-1);
      }
    }
  }
}

TEST(Image, BilinearUpscaleWrapped) {
  FasTC::Pixel pxs[16];

  // Make sure that our bit depth is less than full...
  for(uint32 i = 0; i < 16; i++) {
    const uint8 newBitDepth[4] = { 6, 5, 6, 5 };
    pxs[i].ChangeBitDepth(newBitDepth);
  }

  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i*4;
      pxs[j*4 + i].G() = j*4;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  img.BilinearUpscale(2, 2, PVRTCC::eWrapMode_Wrap);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(16));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(16));

  for(uint32 i = 0; i < img.GetWidth(); i++) {
    for(uint32 j = 0; j < img.GetHeight(); j++) {
      const FasTC::Pixel &p = img(i, j);

      // First make sure that the bit depth didn't change
      uint8 depth[4];
      p.GetBitDepth(depth);
      EXPECT_EQ(depth[0], 6);
      EXPECT_EQ(depth[1], 5);
      EXPECT_EQ(depth[2], 6);
      EXPECT_EQ(depth[3], 5);

      // Now make sure that the values are correct.
      if(i == 0) {
        EXPECT_EQ(p.R(), 6);
      } else if(i == 1) {
        EXPECT_EQ(p.R(), 3);
      } else if(i == 15) {
        EXPECT_EQ(p.R(), 9);
      } else {
        EXPECT_EQ(static_cast<uint32>(p.R()), i-2);
      }

      if(j == 0) {
        EXPECT_EQ(p.G(), 6);
      } else if(j == 1) {
        EXPECT_EQ(p.G(), 3);
      } else if(j == 15) {
        EXPECT_EQ(p.G(), 9);
      } else {
        EXPECT_EQ(static_cast<uint32>(p.G()), j-2);
      }
    }
  }
}

TEST(Image, AverageDownscale) {
  PVRTCC::Image img(8, 8);
  for(uint32 j = 0; j < img.GetHeight(); j++) {
    for(uint32 i = 0; i < img.GetWidth(); i++) {
      if((i ^ j) & 1) {
        img(i, j) = FasTC::Pixel(0xFF000000);
      } else {
        img(i, j) = FasTC::Pixel(0xFFFFFFFF);
      }
    }
  }

  img.AverageDownscale(1, 2);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(4));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(2));

  for(uint32 j = 0; j < img.GetHeight(); j++) {
    for(uint32 i = 0; i < img.GetWidth(); i++) {
      EXPECT_EQ(PixelPrinter(0xFF7F7F7F), PixelPrinter(img(i, j).Pack()));
    }
  }
}

TEST(Image, ContentAwareDownscale) {
  PVRTCC::Image img(8, 8);
  for(uint32 j = 0; j < img.GetHeight(); j++) {
    for(uint32 i = 0; i < img.GetWidth(); i++) {
      if(j < 4) {
        img(i, j) = FasTC::Pixel( 0xFF000000 );
      } else {
        img(i, j) = FasTC::Pixel( 0xFF0000FF );
      }
    }
  }

  img.ContentAwareDownscale(1, 1);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(4));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(4));

  for(uint32 j = 0; j < img.GetHeight(); j++) {
    for(uint32 i = 0; i < img.GetWidth(); i++) {
      if(j < 2) {
        EXPECT_EQ(img(i, j).R(), 0);
      } else {
        EXPECT_EQ(img(i, j).R(), 255);
      }
    }
  }
}

TEST(Image, ChangeBitDepth) {
  PVRTCC::Image img(4, 4);

  uint8 testDepth[4] = { 2, 3, 5, 0 };
  img.ChangeBitDepth(testDepth);

  uint8 depth[4];
  for(uint32 j = 0; j < img.GetHeight(); j++) {
    for(uint32 i = 0; i < img.GetWidth(); i++) {
      img(i, j).GetBitDepth(depth);

      for(int d = 0; d < 4; d++) {
        EXPECT_EQ(testDepth[d], depth[d]);
      }
    }
  }
}
