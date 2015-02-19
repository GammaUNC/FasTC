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
#include "FasTC/Color.h"
#include "FasTC/Image.h"
#include "FasTC/IPixel.h"
#include "FasTC/Pixel.h"
#include "Utils.h"

#include <cstdlib>

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
      EXPECT_EQ(i1(i, j), img(i, j).R());
      EXPECT_EQ(i2(i, j), img(i, j).G());
      EXPECT_EQ(i3(i, j), img(i, j).B());
    }
  }

  FasTC::Image<FasTC::Color> img2(w, h);
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      img2(i, j) = FasTC::Color(j, i, i*j, 255);
    }
  }

  FasTC::SplitChannels(img2, &i1, &i2, &i3);

  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      EXPECT_EQ(i1(i, j), img2(i, j).R());
      EXPECT_EQ(i2(i, j), img2(i, j).G());
      EXPECT_EQ(i3(i, j), img2(i, j).B());
    }
  }
}

TEST(Image, DCT) {

  const uint32 w = 32;
  const uint32 h = 32;

  FasTC::Image<FasTC::IPixel> img(w, h);
  for (uint32 j = 0; j < h; ++j) {
    for (uint32 i = 0; i < w; ++i) {
      img(i, j) = static_cast<FasTC::IPixel>(i + j);
//      img(i, j) = static_cast<FasTC::IPixel>(1);
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
      EXPECT_NEAR(img(i, j), orig(i, j), 1e-5);
    }
  }
}
