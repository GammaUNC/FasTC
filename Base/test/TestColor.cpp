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

static const float kEpsilon = 1e-6f;

TEST(Color, DefaultConstructor) {
  FasTC::Color c;
  EXPECT_EQ(c.R(), 0.0f);
  EXPECT_EQ(c.G(), 0.0f);
  EXPECT_EQ(c.B(), 0.0f);
  EXPECT_EQ(c.A(), 0.0f);
}

TEST(Color, AssignmentConstructor) {
  FasTC::Color c(1.0f, 0.0f, 3.0f, -1.0f);
  EXPECT_EQ(c.R(), 1.0f);
  EXPECT_EQ(c.G(), 0.0f);
  EXPECT_EQ(c.B(), 3.0f);
  EXPECT_EQ(c.A(), -1.0f);
}

TEST(Color, VectorOperators) {
  FasTC::Color a(0.1f, 0.2f, 0.3f, 0.4f);
  FasTC::Color b(0.2f, 0.3f, 0.4f, 0.5f);
  FasTC::Color c = a + b;

  EXPECT_NEAR(c.R(), 0.3, kEpsilon);
  EXPECT_NEAR(c.G(), 0.5, kEpsilon);
  EXPECT_NEAR(c.B(), 0.7, kEpsilon);
  EXPECT_NEAR(c.A(), 0.9, kEpsilon);

  FasTC::Color d = a - b;

  EXPECT_NEAR(d.R(), -0.1, kEpsilon);
  EXPECT_NEAR(d.G(), -0.1, kEpsilon);
  EXPECT_NEAR(d.B(), -0.1, kEpsilon);
  EXPECT_NEAR(d.A(), -0.1, kEpsilon);
}

TEST(Color, EqualityComparison) {
  FasTC::Color a(0.1f, 0.2f, 0.3f, 0.4f);
  FasTC::Color b(0.2f, 0.3f, 0.4f, 0.5f);

  EXPECT_TRUE(a == a && b == b);
  EXPECT_FALSE(a == b && b == a);
}
