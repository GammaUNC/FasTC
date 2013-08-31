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
#include "Image.h"
#include "Pixel.h"

TEST(Image, NonSpecificConstructor) {
  PVRTCC::Pixel p;

  PVRTCC::Image img (4, 4);
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      EXPECT_TRUE(img(i, j) == p);
    }
  }
}

TEST(Image, SpecificConstructor) {
  PVRTCC::Pixel pxs[16];
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i;
      pxs[j*4 + i].G() = j;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      EXPECT_TRUE(img(i, j) == pxs[j*4 + i]);
    }
  }
}

TEST(Image, CopyConstructor) {
  PVRTCC::Pixel pxs[16];
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i;
      pxs[j*4 + i].G() = j;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  PVRTCC::Image img2(img);
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      EXPECT_TRUE(img2(i, j) == pxs[j*4 + i]);
    }
  }
}

TEST(Image, AssignmentOperator) {
  PVRTCC::Pixel pxs[16];
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i;
      pxs[j*4 + i].G() = j;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  PVRTCC::Image img2 = img;
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      EXPECT_TRUE(img2(i, j) == pxs[j*4 + i]);
    }
  }
}

TEST(Image, BilinearUpscale) {
  PVRTCC::Pixel pxs[16];
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i*2;
      pxs[j*4 + i].G() = j*2;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  img.BilinearUpscale(1);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(8));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(8));

  for(uint32 i = 0; i < img.GetWidth(); i++) {
    for(uint32 j = 0; j < img.GetHeight(); j++) {
      if(i == 0) {
        EXPECT_EQ(img(i, j).R(), i);
      } else {
        EXPECT_EQ(img(i, j).R(), i-1);
      }

      if(j == 0) {
        EXPECT_EQ(img(i, j).G(), j);
      } else {
        EXPECT_EQ(img(i, j).G(), j-1);
      }
    }
  }
}

TEST(Image, BilinearUpscaleWrapped) {
  PVRTCC::Pixel pxs[16];
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      pxs[j*4 + i].R() = i*4;
      pxs[j*4 + i].G() = j*4;
    }
  }

  PVRTCC::Image img(4, 4, pxs);
  img.BilinearUpscale(2, PVRTCC::eWrapMode_Wrap);
  EXPECT_EQ(img.GetWidth(), static_cast<uint32>(16));
  EXPECT_EQ(img.GetHeight(), static_cast<uint32>(16));

  for(uint32 i = 0; i < img.GetWidth(); i++) {
    for(uint32 j = 0; j < img.GetHeight(); j++) {
      const PVRTCC::Pixel &p = img(i, j);
      if(i == 0) {
        EXPECT_EQ(p.R(), 6);
      } else if(i == 1) {
        EXPECT_EQ(p.R(), 3);
      } else if(i == 15) {
        EXPECT_EQ(p.R(), 9);
      } else {
        EXPECT_EQ(p.R(), i-2);
      }

      if(j == 0) {
        EXPECT_EQ(p.G(), 6);
      } else if(j == 1) {
        EXPECT_EQ(p.G(), 3);
      } else if(j == 15) {
        EXPECT_EQ(p.G(), 9);
      } else {
        EXPECT_EQ(p.G(), j-2);
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
