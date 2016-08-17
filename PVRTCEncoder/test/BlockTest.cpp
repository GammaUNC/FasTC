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
#include "Block.h"

TEST(Block, ConstructorFromBytes) {
  const uint8 data[8] = { 0 };
  PVRTCC::Block b (data);
  EXPECT_FALSE(b.GetModeBit());
}

TEST(Block, CopyConstructor) {
  const uint8 data[8] = { 0xAA, 0xAA, 0xAA, 0xAA, 0, 0, 0, 0 };
  PVRTCC::Block b (data);
  PVRTCC::Block b2 (b);
  for(int i = 0; i < 16; i++) {
    EXPECT_EQ(b2.GetLerpValue(i), 0x2);
  }
}

TEST(Block, GetColorA) {

  // Test a 555 opaque block
  uint8 data[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8A, 0x96 };
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
  data[7] = 0x16;
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
  uint8 data[8] = { 0x0, 0x0, 0x0, 0x0, 0x8A, 0x96, 0x0, 0x0 };
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
  data[5] = 0x16;
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
  const uint8 modeData[8] = { 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0 };
  const uint8 noModeData[8] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

  PVRTCC::Block b (modeData);
  EXPECT_TRUE(b.GetModeBit());

  b = PVRTCC::Block(noModeData);
  EXPECT_FALSE(b.GetModeBit());
}

TEST(Block, GetLerpValue) {
  const uint8 data[8] = { 0xD8, 0x27, 0xE4, 0x1B, 0x0, 0x0, 0x0, 0x0 };
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

TEST(Block, Get2BPPLerpValue) {
  union {
    uint8 noModDataBytes[8];
    uint32 noModDataInts[2];
  } noModDataVals;
  const uint8 noModData[8] = { 0xDA, 0x27, 0xE4, 0x1B, 0x0, 0x0, 0x0, 0x0 };
  memcpy(noModDataVals.noModDataBytes, noModData, sizeof(noModData));
  PVRTCC::Block b(noModData);

  uint32 noModInt = noModDataVals.noModDataInts[0];
  for(uint32 i = 0; i < 32; i++) {
    EXPECT_EQ(b.Get2BPPLerpValue(i), (noModInt >> i) & 0x1);
  }

  uint8 modData[8];
  memcpy(modData, noModData, sizeof(modData));
  modData[4] = 0x1;

  b = PVRTCC::Block(modData);

  EXPECT_EQ(b.Get2BPPLerpValue(0), 3);
  EXPECT_EQ(b.Get2BPPLerpValue(1), 2);
  EXPECT_EQ(b.Get2BPPLerpValue(2), 1);
  EXPECT_EQ(b.Get2BPPLerpValue(3), 3);

  EXPECT_EQ(b.Get2BPPLerpValue(4), 3);
  EXPECT_EQ(b.Get2BPPLerpValue(5), 1);
  EXPECT_EQ(b.Get2BPPLerpValue(6), 2);
  EXPECT_EQ(b.Get2BPPLerpValue(7), 0);

  EXPECT_EQ(b.Get2BPPLerpValue(8), 0);
  EXPECT_EQ(b.Get2BPPLerpValue(9), 1);
  EXPECT_EQ(b.Get2BPPLerpValue(10), 2);
  EXPECT_EQ(b.Get2BPPLerpValue(11), 3);

  EXPECT_EQ(b.Get2BPPLerpValue(12), 3);
  EXPECT_EQ(b.Get2BPPLerpValue(13), 2);
  EXPECT_EQ(b.Get2BPPLerpValue(14), 1);
  EXPECT_EQ(b.Get2BPPLerpValue(15), 0);
}

TEST(Block, Get2BPPSubMode) {
  uint8 data[8] = { 0xDA, 0x27, 0xE4, 0x1B, 0x1, 0x0, 0x0, 0x0 };
  PVRTCC::Block b(data);

  EXPECT_EQ(b.Get2BPPSubMode(), PVRTCC::Block::e2BPPSubMode_All);

  data[0] = 0xDB;
  b = PVRTCC::Block(data);
  EXPECT_EQ(b.Get2BPPSubMode(), PVRTCC::Block::e2BPPSubMode_Horizontal);

  data[2] = 0xF4;
  b = PVRTCC::Block(data);
  EXPECT_EQ(b.Get2BPPSubMode(), PVRTCC::Block::e2BPPSubMode_Vertical);
}

TEST(Block, SetColorAandB) {
  PVRTCC::Block b;
  PVRTCC::Pixel color;
  color.A() = 212;
  color.R() = 200;
  color.G() = 100;
  color.B() = -120;
  b.SetColorA(color);
  PVRTCC::Pixel cA = b.GetColorA();

  uint8 bitDepth[4] = { 0, 5, 5, 5 };
  color.ChangeBitDepth(bitDepth);

  EXPECT_FALSE(memcmp(&color, &cA, sizeof(color)));
  
  memset(bitDepth, 8, sizeof(bitDepth));
  color.ChangeBitDepth(bitDepth);

  color.A() = 212;
  color.R() = 200;
  color.G() = 100;
  color.B() = -120;
  b.SetColorB(color, true);
  PVRTCC::Pixel cB = b.GetColorB();
  
  uint8 tBitDepth[4] = { 0, 5, 5, 4 };
  color.ChangeBitDepth(tBitDepth);

  EXPECT_FALSE(memcmp(&color, &cB, sizeof(color)));

  memset(bitDepth, 8, sizeof(bitDepth));
  color.ChangeBitDepth(bitDepth);

  color.A() = 100;
  color.R() = 200;
  color.G() = 100;
  color.B() = -120;
  b.SetColorB(color, true);
  PVRTCC::Pixel cC = b.GetColorB();
  
  uint8 uBitDepth[4] = { 3, 4, 4, 3 };
  color.ChangeBitDepth(uBitDepth);

  EXPECT_FALSE(memcmp(&color, &cC, sizeof(color)));
}

TEST(Block, SetLerpValue) {
  PVRTCC::Block b;

  for(int i = 0; i < 16; i++) {
    b.SetLerpValue(i, i%4);
  }

  for(int i = 0; i < 16; i++) {
    EXPECT_EQ(b.GetLerpValue(i), i % 4);
  }
}

TEST(Block, PackBlock) {
  PVRTCC::Block b;

  PVRTCC::Pixel cA, cB;

  cA.A() = 0xFF;
  cA.R() = 0xFF;
  cA.G() = 0x80;
  cA.B() = 0x00;

  cB.A() = 0x80;
  cB.R() = 0x7F;
  cB.G() = 0x00;
  cB.B() = 0xFF;

  b.SetColorA(cA);
  b.SetColorB(cB, true);

  for(int i = 0; i < 16; i++) {
    b.SetLerpValue(i, i%4);
  }

  b.SetModeBit(false);
  EXPECT_EQ(b.Pack(), 0xFE00480EE4E4E4E4UL);

  b.SetModeBit(true);
  EXPECT_EQ(b.Pack(), 0xFE00480FE4E4E4E4UL);

  b.SetColorB(cB);
  b.SetModeBit(false);
  EXPECT_EQ(b.Pack(), 0xFE00C01EE4E4E4E4UL);
}
