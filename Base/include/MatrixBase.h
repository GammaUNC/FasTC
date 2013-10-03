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

#ifndef BASE_INCLUDE_MATRIXBASE_H__
#define BASE_INCLUDE_MATRIXBASE_H__

#include "VectorBase.h"

namespace FasTC {

  template <typename T, const int nRows, const int nCols>
  class MatrixBase {
   protected:

    // Vector representation
    static const int kNumElements = nRows * nCols;
    T mat[kNumElements];

   public:
    
    // Constructors
    MatrixBase() { }
    MatrixBase(const MatrixBase<T, nRows, nCols> &other) {
      for(int i = 0; i < kNumElements; i++) {
        mat[i] = other[i];
      }
    }

    // Accessors
    T &operator()(int idx) { return mat[idx]; }
    const T &operator()(int idx) const { return mat[idx]; }
    T &operator()(int r, int c) { return mat[r * nCols + c]; }
    const T &operator() const (int r, int c) { return mat[r * nCols + c]; }

    T &operator[](int idx) { return mat[idx]; }
    const T &operator[](int idx) const { return mat[idx]; }

    // Operators
    template<typename _T>
    MatrixBase<T, nRows, nCols> operator+(const MatrixBase<_T, nRows, nCols> &m) {
      MatrixBase<T, nRows, nCols> a;
      for(int i = 0; i < kNumElements; i++) {
        a[i] = mat[i] + m[i];
      }
      return a;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> &operator+=(const MatrixBase<_T, nRows, nCols> &m) {
      for(int i = 0; i < kNumElements; i++) {
        mat[i] += m[i];
      }
      return *this;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> operator-(const MatrixBase<_T, nRows, nCols> &m) {
      MatrixBase<T, nRows, nCols> a;
      for(int i = 0; i < kNumElements; i++) {
        a[i] = mat[i] - m[i];
      }
      return a;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> &operator-=(const MatrixBase<_T, nRows, nCols> &m) {
      for(int i = 0; i < kNumElements; i++) {
        mat[i] -= m[i];
      }
      return *this;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> operator*(_T s) {
      MatrixBase<T, nRows, nCols> a;
      for(int i = 0; i < kNumElements; i++) {
        a[i] = mat[i] * s;
      }
      return a;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> &operator*=(_T s) {
      for(int i = 0; i < kNumElements; i++) {
        mat[i] *= s;
      }
      return *this;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> operator/(_T s) {
      MatrixBase<T, nRows, nCols> a;
      for(int i = 0; i < kNumElements; i++) {
        a[i] = mat[i] / s;
      }
      return a;
    }

    template<typename _T>
    MatrixBase<T, nRows, nCols> &operator/=(_T s) {
      for(int i = 0; i < kNumElements; i++) {
        mat[i] /= s;
      }
      return *this;
    }

    // Matrix multiplication
    template<typename _T, const int nTarget>
    MatrixBase<T, nRows, nTarget> operator*(const MatrixBase<_T, nCols, nTarget> &m) {
      MatrixBase<T, nRows, nTarget> result;
      for(int r = 0; r < nRows; r++)
        for(int c = 0; c < nTarget; c++) {
          result(r, c) = 0;
          for(int j = 0; j < nCols; j++) {
            result(r, c) += (*this)(r, j) * m(j, c);
          }
        }
      return result;
    }

    // Vector multiplication -- treat vectors as Nx1 matrices...
    template<typename _T>
    VectorBase<T, nCols> operator*(const VectorBase<_T, nCols> &v) {
      VectorBase<T, nCols> result;
      for(int r = 0; r < nRows; r++) {
        result(r) = 0;
        for(int j = 0; j < nCols; j++) {
          result(r) += (*this)(r, j) * v(j);
        }
      }
      return result;
    }

    // Outer product...
    template<typename _T, typename _U, const int N, const int M>
    friend MatrixBase<_T, N, M> operator^(
      const VectorBase<_T, N> &a, 
      const VectorBase<_U, M> &b
    ) {
      MatrixBase<_T, N, M> result;

      for(int i = 0; i < N; i++)
        for(int j = 0; j < M; j++)
          result(i, j) = a[i] * b[j];
 
      return result;
    }

    template<typename _T, typename _U, const int N, const int M>
    friend MatrixBase<_T, N, M> OuterProduct(
      const VectorBase<_T, N> &a, 
      const VectorBase<_U, M> &b
    ) { 
      return a ^ b; 
    }

    // Double dot product
    template<typename _T>
    T DDot(const MatrixBase<_T, nRows, nCols> &m) {
      T result = 0;
      for(int i = 0; i < kNumElements; i++) {
        result += mat[i] * m[i];
      }
      return result;
    }

  };
};

#endif  // BASE_INCLUDE_MATRIXBASE_H_
