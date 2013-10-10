/*******************************************************************************
 * Copyright (c) 2012 Pavel Krajcevski
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 *
 ******************************************************************************/

#ifndef BASE_INCLUDE_MATRIXSQUARE_H_
#define BASE_INCLUDE_MATRIXSQUARE_H_

#include "MatrixBase.h"

namespace FasTC {

  template <typename T, const int N>
  class MatrixSquare : public MatrixBase<T, N, N> {
   public:
    
    // Constructors
    MatrixSquare() { }
    MatrixSquare(const MatrixBase<T, N, N> &other) {
      for(int i = 0; i < kNumElements; i++) {
        mat[i] = other[i];
      }
    }
  };
};

#endif  // BASE_INCLUDE_MATRIXSQUARE_H_
