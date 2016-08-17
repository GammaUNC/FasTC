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
#include "IntegerEncoding.h"
using ASTCC::IntegerEncodedValue;

TEST(IntegerEncoding, GetEncoding) {
  // According to table C.2.7
  IntegerEncodedValue val = IntegerEncodedValue::CreateEncoding(1);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 1U);

  val = IntegerEncodedValue::CreateEncoding(2);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 0U);

  val = IntegerEncodedValue::CreateEncoding(3);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 2U);

  val = IntegerEncodedValue::CreateEncoding(4);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Quint);
  EXPECT_EQ(val.BaseBitLength(), 0U);

  val = IntegerEncodedValue::CreateEncoding(5);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 1U);

  val = IntegerEncodedValue::CreateEncoding(7);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 3U);

  val = IntegerEncodedValue::CreateEncoding(9);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Quint);
  EXPECT_EQ(val.BaseBitLength(), 1U);

  val = IntegerEncodedValue::CreateEncoding(11);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 2U);

  val = IntegerEncodedValue::CreateEncoding(15);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 4U);

  val = IntegerEncodedValue::CreateEncoding(19);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Quint);
  EXPECT_EQ(val.BaseBitLength(), 2U);

  val = IntegerEncodedValue::CreateEncoding(23);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_Trit);
  EXPECT_EQ(val.BaseBitLength(), 3U);

  val = IntegerEncodedValue::CreateEncoding(31);
  EXPECT_EQ(val.GetEncoding(), ASTCC::eIntegerEncoding_JustBits);
  EXPECT_EQ(val.BaseBitLength(), 5U);
}

