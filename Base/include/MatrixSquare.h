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
#include <cstdlib>
#include <ctime>

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

    MatrixSquare<T, N> Transpose() const {
      return MatrixBase<T, N, N>::Transpose();
    }

    // Does power iteration to determine the principal eigenvector and eigenvalue.
    // Returns them in eigVec and eigVal after kMaxNumIterations
    int PowerMethod(VectorBase<T, N> &eigVec,
                    T *eigVal = NULL,
                    const int kMaxNumIterations = 5) {

      int numIterations = 0;

      VectorBase<T, N> b;
      T norm = 1.0/sqrt(static_cast<T>(N));
      for(int i = 0; i < N; i++)
        b[i] = norm;

      bool badEigenValue = false;
      bool fixed = false;
      numIterations = 0;
      while(!fixed && ++numIterations < kMaxNumIterations) {

        VectorBase<T, N> newB = (*this) * b;

        // !HACK! If the principal eigenvector of the matrix
        // converges to zero, that could mean that there is no
        // principal eigenvector. However, that may be due to
        // poor initialization of the random vector, so rerandomize
        // and try again.
        const T newBlen = newB.Length();
        if(newBlen < 1e-10) {
          if(badEigenValue) {
            eigVec = b;
            if(eigVal) *eigVal = 0.0;
            return numIterations;
          }

          VectorBase<T, N> b;
          for(int i = 0; i < (N>>1); i++)
            b[i] = 1;

          b.Normalize();
          badEigenValue = true;
        }

        // Normalize
        newB.Normalize();

        // If the new eigenvector is close enough to the old one,
        // then we've converged.
        if(fabs(1.0f - (b.Dot(newB))) < 1e-8)
          fixed = true;

        // Save and continue.
        b = newB;
      }

      // Store the eigenvector in the proper variable.
      eigVec = b;

      // Store eigenvalue if it was requested
      if(eigVal) {
        VectorBase<T, N> result = (*this) * b;
        *eigVal = result.Length() / b.Length();
      }

      return numIterations;
    }

   private:

  };
  REGISTER_ONE_TEMPLATE_MATRIX_SIZED_TYPE(MatrixSquare);

};

#endif  // BASE_INCLUDE_MATRIXSQUARE_H_
