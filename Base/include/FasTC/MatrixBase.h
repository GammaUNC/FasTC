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

    // Equality operator 
    template<typename _T>
    bool operator==(MatrixBase<_T, nRows, nCols> &other) const { 
      bool result = true;
      for(int i = 0; i < Size; i++) {
        result = result && (mat[i] == other[i]);
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

  // Matrix multiplication
  template<typename T, typename _T, const int nRows, const int nCols, const int nTarget>
  inline MatrixBase<T, nRows, nTarget>
    MultiplyMatrix(const MatrixBase<T, nRows, nCols> &a,
                   const MatrixBase<_T, nCols, nTarget> &b) {
    MatrixBase<T, nRows, nTarget> result;
    for(int r = 0; r < nRows; r++)
    for(int c = 0; c < nTarget; c++) {
      result(r, c) = 0;
      for(int j = 0; j < nCols; j++) {
        result(r, c) += a(r, j) * b(j, c);
      }
    }
    return result;
  }

  // Vector multiplication -- treat vectors as Nx1 matrices...
  template<typename T, typename _T, int nRows, int nCols>
  inline VectorBase<T, nCols> VectorMatrixMultiply(const VectorBase<T, nRows> &v,
                                                   const MatrixBase<_T, nRows, nCols> &m) {
    VectorBase<T, nCols> r;
    for(int j = 0; j < nCols; j++) {
      r(j) = 0;
      for(int i = 0; i < nRows; i++) {
        r(j) += m(i, j) * v(i);
      }
    }
    return r;
  }

  template<typename T, typename _T, int nRows, int nCols>
  inline VectorBase<T, nRows> MatrixVectorMultiply(const MatrixBase<_T, nRows, nCols> &m,
                                                   const VectorBase<T, nCols> &v) {
    VectorBase<T, nRows> r;
    for(int j = 0; j < nRows; j++) {
      r(j) = 0;
      for(int i = 0; i < nCols; i++) {
        r(j) += m(j, i) * v(i);
      }
    }
    return r;
  }

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

  #define REGISTER_ONE_TEMPLATE_MATRIX_SIZED_TYPE(TYPE)        \
  template<typename T, const int SIZE>                         \
  class VectorTraits< TYPE <T, SIZE> > {                       \
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

    ResultType GetMultiplication() const { return MatrixVectorMultiply(m_A, m_B); }
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

    ResultType GetMultiplication() const { return VectorMatrixMultiply(m_A, m_B); }
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

    ResultType GetMultiplication() const { return MultiplyMatrix(m_A, m_B); }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Matrix,
    eVectorType_Scalar,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef MatrixBase<typename TypeOne::ScalarType, TypeOne::kNumRows, TypeOne::kNumCols> ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const {
      TypeOne result;
      for(int i = 0; i < TypeOne::Size; i++) {
        result[i] = m_A[i] * m_B;
      }
      return result;
    }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Scalar,
    eVectorType_Matrix,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef MatrixBase<typename TypeTwo::ScalarType, TypeTwo::kNumRows, TypeTwo::kNumCols> ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const {
      TypeTwo result;
      for(int i = 0; i < TypeTwo::Size; i++) {
        result[i] = m_A * m_B[i];
      }
      return result;
    }
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
