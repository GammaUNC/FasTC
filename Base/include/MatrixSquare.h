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
    MatrixSquare(const MatrixSquare<T, N> &other)
      : MatrixBase<T, N, N>(other) { }
    MatrixSquare(const MatrixBase<T, N, N> &other)
      : MatrixBase<T, N, N>(other) { }

    // Does power iteration to determine the principal eigenvector and eigenvalue.
    // Returns them in eigVec and eigVal after kMaxNumIterations
    int PowerMethod(VectorBase<T, N> &eigVec, T *eigVal = NULL,
                    const int kMaxNumIterations = 200) {
      int numIterations = 0;

      // !SPEED! Find eigenvectors by using the power method. This is good because the
      // matrix is only 4x4, which allows us to use SIMD...
      VectorBase<T, N> b;
      for(int i = 0; i < N; i++)
        b[i] = T(1.0);
      
      b /= b.Length();

      bool fixed = false;
      numIterations = 0;
      while(!fixed && ++numIterations < kMaxNumIterations) {

        VectorBase<T, N> newB = (*this).operator*(b);

        // !HACK! If the principal eigenvector of the covariance matrix
        // converges to zero, that means that the points lie equally 
        // spaced on a sphere in this space. In this (extremely rare)
        // situation, just choose a point and use it as the principal 
        // direction.
        const float newBlen = newB.Length();
        if(newBlen < 1e-10) {
          eigVec = b;
          if(eigVal) *eigVal = 0.0;
          return numIterations;
        }

        T len = newB.Length();
        newB /= len;
        if(eigVal)
          *eigVal = len;

        if(fabs(1.0f - (b.Dot(newB))) < 1e-5)
          fixed = true;

        b = newB;
      }

      eigVec = b;  
      return numIterations;
    }

  };
};

#endif  // BASE_INCLUDE_MATRIXSQUARE_H_
