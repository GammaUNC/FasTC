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

#include "FasTC/Shapes.h"

#include <cassert>

static const int kAnchorIdx2[BPTCC::kNumShapes2] = {
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15,  2,  8,  2,  2,  8,  8, 15,
  2 ,  8,  2,  2,  8,  8,  2,  2,
  15, 15,  6,  8,  2,  8, 15, 15,
  2 ,  8,  2,  2,  2, 15, 15,  6,
  6 ,  2,  6,  8, 15, 15,  2,  2,
  15, 15, 15, 15, 15,  2,  2, 15
};

static const int kAnchorIdx3[2][BPTCC::kNumShapes3] = {
  {3,  3, 15, 15,  8,  3, 15, 15,
  8 ,  8,  6,  6,  6,  5,  3,  3,
  3 ,  3,  8, 15,  3,  3,  6, 10,
  5 ,  8,  8,  6,  8,  5, 15, 15,
  8 , 15,  3,  5,  6, 10,  8, 15,
  15,  3, 15,  5, 15, 15, 15, 15,
  3 , 15,  5,  5,  5,  8,  5, 10,
  5 , 10,  8, 13, 15, 12,  3,  3 },

  {15,  8,  8,  3, 15, 15,  3,  8,
  15 , 15, 15, 15, 15, 15, 15,  8,
  15 ,  8, 15,  3, 15,  8, 15,  8,
  3  , 15,  6, 10, 15, 15, 10,  8,
  15 ,  3, 15, 10, 10,  8,  9, 10,
  6  , 15,  8, 15,  3,  6,  6,  8,
  15 ,  3, 15, 15, 15, 15, 15, 15,
  15 , 15, 15, 15,  3, 15, 15,  8 }
};

namespace BPTCC {

static uint32 GetAnchorIndexForSubset(
  int subset, const int shapeIdx, const int nSubsets
) {

  int anchorIdx = 0;
  switch(subset) {
    case 1:
    {
      if(nSubsets == 2) {
        anchorIdx = kAnchorIdx2[shapeIdx];
      } else {
        anchorIdx = kAnchorIdx3[0][shapeIdx];
      }
    }
    break;

    case 2:
    {
      assert(nSubsets == 3);
      anchorIdx = kAnchorIdx3[1][shapeIdx];
    }
    break;

    default:
    break;
  }

  return anchorIdx;
}

}  // namespace BPTCC
