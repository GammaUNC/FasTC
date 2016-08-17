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

#ifndef BASE_INCLUDE_MATRIX2X2_H_
#define BASE_INCLUDE_MATRIX2X2_H_

#include "MatrixSquare.h"

namespace FasTC {

  template <typename T>
  class Matrix2x2 : public MatrixSquare<T, 2> {
   public:
    // Constructors
    Matrix2x2() { }
    Matrix2x2(const Matrix2x2<T> &other)
      : MatrixSquare<T, 2>(other) { }
    Matrix2x2(const MatrixSquare<T, 2> &other)
      : MatrixSquare<T, 2>(other) { }
    Matrix2x2(const MatrixBase<T, 2, 2> &other)
      : MatrixSquare<T, 2>(other) { }
  };
  REGISTER_ONE_TEMPLATE_MATRIX_TYPE(Matrix2x2);
};

#endif  // BASE_INCLUDE_MATRIX2X2_H_
