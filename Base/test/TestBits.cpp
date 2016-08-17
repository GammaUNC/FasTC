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
#include "FasTC/Bits.h"

TEST(Bits, Replicate) {
  uint32 xv = 3;
  EXPECT_EQ(FasTC::Replicate(xv, 2, 8), 0xFFU);
  EXPECT_EQ(FasTC::Replicate(xv, 0, 7), 0U);
  EXPECT_EQ(FasTC::Replicate(xv, 3, 4), 0x06U);

  xv = 0;
  EXPECT_EQ(FasTC::Replicate(xv, 1, 0), 0U);
  EXPECT_EQ(FasTC::Replicate(xv, 0, 7), 0U);

  xv = 5;
  EXPECT_EQ(FasTC::Replicate(xv, 2, 0), 0U);
  EXPECT_EQ(FasTC::Replicate(xv, 3, 6), 0x2DU);
}
