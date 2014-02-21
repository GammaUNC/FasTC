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
    T mat[nRows * nCols];

   public:
    typedef T ScalarType;
    static const int kNumRows = nRows;
    static const int kNumCols = nCols;
    static const int Size = kNumCols * kNumRows;

    // Constructors
    MatrixBase() { }
    MatrixBase(const MatrixBase<T, nRows, nCols> &other) {
      for(int i = 0; i < Size; i++) {
        (*this)[i] = other[i];
      }
    }

    // Accessors
    T &operator()(int idx) { return mat[idx]; }
    T &operator[](int idx) { return mat[idx]; }
    const T &operator()(int idx) const { return mat[idx]; }
    const T &operator[](int idx) const { return mat[idx]; }

    T &operator()(int r, int c) { return (*this)[r * nCols + c]; }
    const T &operator() (int r, int c) const { return (*this)[r * nCols + c]; }

    // Allow casts to the respective array representation...
    operator const T *() const { return this->mat; }
    MatrixBase<T, nRows, nCols> &operator=(const T *v) {
      for(int i = 0; i < Size; i++)
        (*this)[i] = v[i];
      return *this;
    }

    // Allows casting to other vector types if the underlying type system does as well...
    template<typename _T>
    operator MatrixBase<_T, nRows, nCols>() const { 
      MatrixBase<_T, nRows, nCols> ret;
      for(int i = 0; i < Size; i++) {
        ret[i] = static_cast<_T>(mat[i]);
      }
      return ret;
    }

    // Matrix multiplication
    template<typename _T, const int nTarget>
    MatrixBase<T, nRows, nTarget> MultiplyMatrix(const MatrixBase<_T, nCols, nTarget> &m) const {
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
    VectorBase<T, nCols> MultiplyVectorLeft(const VectorBase<_T, nRows> &v) const {
      VectorBase<T, nCols> result;
      for(int j = 0; j < nCols; j++) {
        result(j) = 0;
        for(int r = 0; r < nRows; r++) {
          result(j) += (*this)(r, j) * v(r);
        }
      }
      return result;
    }

    template<typename _T>
    VectorBase<T, nRows> MultiplyVectorRight(const VectorBase<_T, nCols> &v) const {
      VectorBase<T, nRows> result;
      for(int r = 0; r < nRows; r++) {
        result(r) = 0;
        for(int j = 0; j < nCols; j++) {
          result(r) += (*this)(r, j) * v(j);
        }
      }
      return result;
    }

    // Transposition
    MatrixBase<T, nCols, nRows> Transpose() const {
      MatrixBase<T, nCols, nRows> result;
      for(int r = 0; r < nRows; r++) {
        for(int c = 0; c < nCols; c++) {
          result(c, r) = (*this)(r, c);
        }
      }
      return result;
    }

    // Double dot product
    template<typename _T>
    T DDot(const MatrixBase<_T, nRows, nCols> &m) const {
      T result = 0;
      for(int i = 0; i < Size; i++) {
        result += (*this)[i] * m[i];
      }
      return result;
    }
  };

  template<typename T, const int N, const int M>
  class VectorTraits<MatrixBase<T, N, M> > {
   public:
    static const EVectorType kVectorType = eVectorType_Matrix;
  };

  #define REGISTER_MATRIX_TYPE(TYPE)                           \
  template<>                                                   \
  class VectorTraits< TYPE > {                                 \
  public:                                                      \
    static const EVectorType kVectorType = eVectorType_Matrix; \
  }

  #define REGISTER_ONE_TEMPLATE_MATRIX_TYPE(TYPE)              \
  template<typename T>                                         \
  class VectorTraits< TYPE <T> > {                             \
  public:                                                      \
    static const EVectorType kVectorType = eVectorType_Matrix; \
  }

  // Define matrix multiplication for * operator
  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Matrix,
    eVectorType_Vector,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef VectorBase<typename TypeTwo::ScalarType, TypeOne::kNumRows> ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return m_A.MultiplyVectorRight(m_B); }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Vector,
    eVectorType_Matrix,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef VectorBase<typename TypeOne::ScalarType, TypeTwo::kNumCols> ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return m_B.MultiplyVectorLeft(m_A); }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Matrix,
    eVectorType_Matrix,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef MatrixBase<typename TypeOne::ScalarType, TypeOne::kNumRows, TypeTwo::kNumCols> ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return m_A.MultiplyMatrix(m_B); }
  };

  // Outer product...
  template<typename _T, typename _U, const int N, const int M>
  MatrixBase<_T, N, M> operator^(
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
  MatrixBase<_T, N, M> OuterProduct(
    const VectorBase<_T, N> &a, 
    const VectorBase<_U, M> &b
  ) { 
    return a ^ b; 
  }

};

#endif  // BASE_INCLUDE_MATRIXBASE_H_
