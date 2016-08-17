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
      T norm = static_cast<T>(1.0)/sqrt(static_cast<T>(N));
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

          for(int i = 0; i < (N>>1); i++)
            b[i] = 1;

          b.Normalize();
          badEigenValue = true;
          continue;
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
