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
#include "FasTC/Color.h"
#include "FasTC/Image.h"
#include "FasTC/IPixel.h"
#include "FasTC/Pixel.h"
#include "Utils.h"

#include <cstdlib>
#include <functional>

TEST(Image, NonSpecificConstructor) {
  FasTC::Pixel p;
  FasTC::Image<FasTC::Pixel> img (4, 4);
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      EXPECT_TRUE(img(i, j) == p);
    }
  }
}

TEST(Image, SpecificConstructor) {
  FasTC::Pixel pxs[16];
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i;
      pxs[j*4 + i].G() = j;
    }
  }

  FasTC::Image<FasTC::Pixel> img(4, 4, pxs);
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      EXPECT_TRUE(img(i, j) == pxs[j*4 + i]);
    }
  }
}

TEST(Image, CopyConstructor) {
  FasTC::Pixel pxs[16];
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i;
      pxs[j*4 + i].G() = j;
    }
  }

  FasTC::Image<FasTC::Pixel> img(4, 4, pxs);
  FasTC::Image<FasTC::Pixel> img2(img);
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      EXPECT_TRUE(img2(i, j) == pxs[j*4 + i]);
    }
  }
}

TEST(Image, AssignmentOperator) {
  FasTC::Pixel pxs[16];
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i;
      pxs[j*4 + i].G() = j;
    }
  }

  FasTC::Image<FasTC::Pixel> img(4, 4, pxs);
  FasTC::Image<FasTC::Pixel> img2 = img;
  for(uint32 i = 0; i < 4; i++) {
    for(uint32 j = 0; j < 4; j++) {
      EXPECT_TRUE(img2(i, j) == pxs[j*4 + i]);
    }
  }
}

TEST(Image, Filter) {
  const uint32 w = 16;
  const uint32 h = 16;

  // Make a black and white image...
  FasTC::Image<FasTC::IPixel> img(w, h);
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      if((i ^ j) % 2)
        img(i, j) = 1.0f;
      else
        img(i, j) = 0.0f;
    }
  }

  // Make a weird averaging kernel...
  FasTC::Image<FasTC::IPixel> kernel(3, 3);
  kernel(0, 1) = kernel(1, 0) = kernel(1, 2) = kernel(2, 1) = 0.125f;
  kernel(1, 1) = 0.5f;

  img.Filter(kernel);

  for(uint32 j = 1; j < h-1; j++) {
    for(uint32 i = 1; i < w-1; i++) {
      EXPECT_NEAR(static_cast<float>(img(i, j)), 0.5f, 0.01);
    }
  }
}

TEST(Image, ComputeMSSIM) {

  const uint32 w = 16;
  const uint32 h = 16;

  FasTC::Image<FasTC::IPixel> img(w, h);
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      img(i, j) = static_cast<float>(
        (static_cast<double>(i) * static_cast<double>(j)) /
        (static_cast<double>(w) * static_cast<double>(h)));
    }
  }

  double SSIM = img.ComputeSSIM(&img);
  EXPECT_EQ(SSIM, 1.0);
}

TEST(Image, SplitImage) {

  const uint32 w = 16;
  const uint32 h = 16;

  FasTC::Image<FasTC::Pixel> img(w, h);
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      img(i, j) = FasTC::Pixel(i, j, i+j, 255);
    }
  }

  FasTC::Image<FasTC::IPixel> i1(w, h);
  FasTC::Image<FasTC::IPixel> i2(w, h);
  FasTC::Image<FasTC::IPixel> i3(w, h);
  FasTC::SplitChannels(img, &i1, &i2, &i3);

  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      EXPECT_FLOAT_EQ(i1(i, j), img(i, j).R());
      EXPECT_FLOAT_EQ(i2(i, j), img(i, j).G());
      EXPECT_FLOAT_EQ(i3(i, j), img(i, j).B());
    }
  }

  FasTC::Image<FasTC::Color> img2(w, h);
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      const float r = static_cast<float>(j);
      const float g = static_cast<float>(i);
      const float b = static_cast<float>(i*j);
      const float a = 255.0f;
      img2(i, j) = FasTC::Color(r, g, b, a);
    }
  }

  FasTC::SplitChannels(img2, &i1, &i2, &i3);

  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      EXPECT_FLOAT_EQ(i1(i, j), img2(i, j).R());
      EXPECT_FLOAT_EQ(i2(i, j), img2(i, j).G());
      EXPECT_FLOAT_EQ(i3(i, j), img2(i, j).B());
    }
  }
}

TEST(Image, DCT) {
  const uint32 w = 32;
  const uint32 h = 32;

  FasTC::Image<FasTC::IPixel> img(w, h);
  for (uint32 j = 0; j < h; ++j) {
    for (uint32 i = 0; i < w; ++i) {
      img(i, j) = static_cast<FasTC::IPixel>(1);
    }
  }

  // Make sure that taking the DCT and inverse DCT returns
  // the same image...
  FasTC::DiscreteCosineXForm(&img, 8);

  // First make sure they're different
  for (uint32 j = 0; j < h; ++j) {
    for (uint32 i = 0; i < w; ++i) {
      if ( (i % 8) == 0 && (j % 8) == 0 ) {
        EXPECT_NEAR(img(i, j), 8.0f, 1e-5);
      } else {
        EXPECT_NEAR(img(i, j), 0.0f, 1e-5);
      }
    }
  }  
}

TEST(Image, IDCT) {

  const uint32 w = 32;
  const uint32 h = 32;

  FasTC::Image<FasTC::IPixel> img(w, h);
  for (uint32 j = 0; j < h; ++j) {
    for (uint32 i = 0; i < w; ++i) {
      img(i, j) = static_cast<float>(i + j);
    }
  }

  FasTC::Image<FasTC::IPixel> orig(img);

  // Make sure that taking the DCT and inverse DCT returns
  // the same image...
  FasTC::DiscreteCosineXForm(&img, 8);

  // First make sure they're different
  for (uint32 j = 0; j < h; ++j) {
    for (uint32 i = 0; i < w; ++i) {
      EXPECT_NE(img(i, j), orig(i, j));
    }
  }  
  
  FasTC::InvDiscreteCosineXForm(&img, 8);

  for (uint32 j = 0; j < h; ++j) {
    for (uint32 i = 0; i < w; ++i) {
      EXPECT_NEAR(img(i, j), orig(i, j), 1e-4);
    }
  }
}
