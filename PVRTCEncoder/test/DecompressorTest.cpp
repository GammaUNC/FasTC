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

#include "TestUtils.h"

#include "FasTC/PVRTCCompressor.h"

static const FasTC::ECompressionFormat kFmt = FasTC::eCompressionFormat_PVRTC4;

TEST(Decompressor, DecompressWhite) {
  const uint32 kWidth = 32;
  const uint32 kHeight = 32;

  uint8 pvrData[512];

  for(int i = 0; i < 512; i += 8) {
    uint8 whiteBlock[8] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xFE, 0xFF, 0xFF, 0xFF };
    memcpy(pvrData + i, whiteBlock, 8);
  }

  uint8 outData[4 * kWidth * kHeight];

  FasTC::DecompressionJob dcj (kFmt, pvrData, outData, kWidth, kHeight);
  PVRTCC::Decompress(dcj);

  for(uint32 i = 0; i < kWidth; i++) {
    for(uint32 j = 0; j < kHeight; j++) {
      const uint32 *pixelData = reinterpret_cast<const uint32 *>(outData);
      const uint32 p = pixelData[j*kWidth + i];
      EXPECT_EQ(PixelPrinter(p), PixelPrinter(0xFFFFFFFF));
    }
  }
}

TEST(Decompressor, DecompressGray) {
  const uint32 kWidth = 32;
  const uint32 kHeight = 32;

  uint8 pvrData[512];

  for(uint32 i = 0; i < 512; i += 8) {
    uint8 grayBlock[8] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xF0, 0xBD, 0x0F, 0xC2 };
    memcpy(pvrData + i, grayBlock, 8);
  }

  uint8 outData[4 * kWidth * kHeight];

  FasTC::DecompressionJob dcj (kFmt, pvrData, outData, kWidth, kHeight);
  PVRTCC::Decompress(dcj);

  for(uint32 i = 0; i < kWidth; i++) {
    for(uint32 j = 0; j < kHeight; j++) {
      const uint32 *pixelData = reinterpret_cast<const uint32 *>(outData);
      const uint32 p = pixelData[j*kWidth + i];
      EXPECT_EQ(PixelPrinter(p), PixelPrinter(0xFF818080));
    }
  }
}
