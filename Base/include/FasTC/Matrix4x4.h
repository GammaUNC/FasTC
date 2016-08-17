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

#ifndef BASE_INCLUDE_MATRIX4X4_H_
#define BASE_INCLUDE_MATRIX4X4_H_

#include "MatrixSquare.h"

namespace FasTC {

  template <typename T>
  class Matrix4x4 : public MatrixSquare<T, 4> {
   public:
    // Constructors
    Matrix4x4() { }
    Matrix4x4(const Matrix4x4<T> &other)
      : MatrixSquare<T, 4>(other) { }
    Matrix4x4(const MatrixSquare<T, 4> &other)
      : MatrixSquare<T, 4>(other) { }
    Matrix4x4(const MatrixBase<T, 4, 4> &other)
      : MatrixSquare<T, 4>(other) { }
  };
  REGISTER_ONE_TEMPLATE_MATRIX_TYPE(Matrix4x4);
};

#endif  // BASE_INCLUDE_MATRIX3X3_H_
