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
#include "Block.h"

TEST(Block, ConstructorFromBytes) {
  const uint8 data[8] = { 0 };
  PVRTCC::Block b (data);
  EXPECT_FALSE(b.GetModeBit());
}

TEST(Block, CopyConstructor) {
  const uint8 data[8] = { 0, 0, 0, 0, 0xAA, 0xAA, 0xAA, 0xAA };
  PVRTCC::Block b (data);
  PVRTCC::Block b2 (b);
  for(int i = 0; i < 16; i++) {
    EXPECT_EQ(b2.GetLerpValue(i), 0x2);
  }
}

TEST(Block, GetColorA) {

  // Test a 555 opaque block
  uint8 data[8] = { 0x96, 0x8A, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
  PVRTCC::Block b(data);
  PVRTCC::Pixel p = b.GetColorA();
  EXPECT_EQ(p.A(), 255);
  EXPECT_EQ(p.R(), 5);
  EXPECT_EQ(p.G(), 20);
  EXPECT_EQ(p.B(), 10);

  uint8 depth[4];
  p.GetBitDepth(depth);
  for(int i = 0; i < 4; i++) {
    if(i == 0) {
      EXPECT_EQ(depth[i], 0);
    } else {
      EXPECT_EQ(depth[i], 5);
    }
  }

  // Test a 3444 transparent block
  data[0] = 0x16;
  b = PVRTCC::Block(data);
  p = b.GetColorA();
  EXPECT_EQ(p.A(), 1);
  EXPECT_EQ(p.R(), 6);
  EXPECT_EQ(p.G(), 8);
  EXPECT_EQ(p.B(), 10);

  p.GetBitDepth(depth);
  for(int i = 0; i < 4; i++) {
    if(i == 0) {
      EXPECT_EQ(depth[i], 3);
    } else {
      EXPECT_EQ(depth[i], 4);
    }
  }
}

TEST(Block, GetColorB) {

  // Test a 554 opaque block
  uint8 data[8] = { 0x0, 0x0, 0x96, 0x8A, 0x0, 0x0, 0x0, 0x0 };
  PVRTCC::Block b(data);
  PVRTCC::Pixel p = b.GetColorB();
  EXPECT_EQ(p.A(), 255);
  EXPECT_EQ(p.R(), 5);
  EXPECT_EQ(p.G(), 20);
  EXPECT_EQ(p.B(), 5);

  uint8 depth[4];
  p.GetBitDepth(depth);
  for(int i = 0; i < 4; i++) {
    if(i == 0) {
      EXPECT_EQ(depth[i], 0);
    } else if(i == 3) {
      EXPECT_EQ(depth[i], 4);
    } else {
      EXPECT_EQ(depth[i], 5);
    }
  }

  // Test a 3443 transparent block
  data[2] = 0x16;
  b = PVRTCC::Block(data);
  p = b.GetColorB();
  EXPECT_EQ(p.A(), 1);
  EXPECT_EQ(p.R(), 6);
  EXPECT_EQ(p.G(), 8);
  EXPECT_EQ(p.B(), 5);

  p.GetBitDepth(depth);
  for(int i = 0; i < 4; i++) {
    if(i == 0 || i == 3) {
      EXPECT_EQ(depth[i], 3);
    } else {
      EXPECT_EQ(depth[i], 4);
    }
  }
}

TEST(Block, GetModeBit) {
  const uint8 modeData[8] = { 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0 };
  const uint8 noModeData[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

  PVRTCC::Block b (modeData);
  EXPECT_TRUE(b.GetModeBit());

  b = PVRTCC::Block(noModeData);
  EXPECT_FALSE(b.GetModeBit());
}

TEST(Block, GetLerpValue) {
  const uint8 data[8] = { 0x0, 0x0, 0x0, 0x0, 0x1B, 0xE4, 0x27, 0xD8 };
  PVRTCC::Block b(data);

  EXPECT_EQ(b.GetLerpValue(0), 0);
  EXPECT_EQ(b.GetLerpValue(1), 2);
  EXPECT_EQ(b.GetLerpValue(2), 1);
  EXPECT_EQ(b.GetLerpValue(3), 3);

  EXPECT_EQ(b.GetLerpValue(4), 3);
  EXPECT_EQ(b.GetLerpValue(5), 1);
  EXPECT_EQ(b.GetLerpValue(6), 2);
  EXPECT_EQ(b.GetLerpValue(7), 0);

  EXPECT_EQ(b.GetLerpValue(8), 0);
  EXPECT_EQ(b.GetLerpValue(9), 1);
  EXPECT_EQ(b.GetLerpValue(10), 2);
  EXPECT_EQ(b.GetLerpValue(11), 3);

  EXPECT_EQ(b.GetLerpValue(12), 3);
  EXPECT_EQ(b.GetLerpValue(13), 2);
  EXPECT_EQ(b.GetLerpValue(14), 1);
  EXPECT_EQ(b.GetLerpValue(15), 0);
}
