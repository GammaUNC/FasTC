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

#ifndef BASE_INCLUDE_VECTORBASE_H_
#define BASE_INCLUDE_VECTORBASE_H_

// !FIXME! For sqrt function. This increases compilation time by a LOT 
// but I couldn't guarantee any faster general-purpose implementation
#include <cmath>

namespace FasTC {

  template <typename T, const int N>
  class VectorBase {
   protected:

    // Vector representation
    T vec[N];

   public:
    
    VectorBase() { }
    VectorBase(const VectorBase<T, N> &other) {
      for(int i = 0; i < N; i++) vec[i] = other[i];
    }

    explicit VectorBase(T *_vec) {
      for(int i = 0; i < N; i++) {
        vec[i] = _vec[i];
      }
    }

    static const int Size = N;

    // Accessors
    T &operator()(int idx) { return vec[idx]; }
    T &operator[](int idx) { return vec[idx]; }
    const T &operator()(int idx) const { return vec[idx]; }
    const T &operator[](int idx) const { return vec[idx]; }

    // Allow casts to the respective array representation...
    operator T *() const { return vec; }
    VectorBase<T, N> &operator=(const T *v) {
      for(int i = 0; i < N; i++)
        vec[i] = v[i];
      return *this;
    }

    // Allows casting to other vector types if the underlying type system does as well...
    template<typename _T>
    operator VectorBase<_T, N>() const { 
      return VectorBase<_T, N>(vec); 
    }

    // Vector operations
    template<typename _T>
    T Dot(const VectorBase<_T, N> &v) const {
      T sum = 0;
      for(int i = 0; i < N; i++)
        sum += vec[i] * v[i];
      return sum;
    }

    T LengthSq() const { return this->Dot(*this); }
    T Length() const { return sqrt(LengthSq()); }
  };

  // Operators
  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne operator+(const VectorTypeOne &v1,
                                        const VectorTypeTwo &v2) {
    VectorTypeOne a;
    for(int i = 0; i < VectorTypeOne::Size; i++)
      a(i) = v1(i) + v2(i);
    return a;
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne &operator+=(VectorTypeOne &v1,
                                          const VectorTypeTwo &v2) {
    for(int i = 0; i < VectorTypeOne::Size; i++)
      v1(i) += v2(i);
    return v1;
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne operator-(const VectorTypeOne &v1,
                                        const VectorTypeTwo &v2) {
    VectorTypeOne a;
    for(int i = 0; i < VectorTypeOne::Size; i++)
      a(i) = v1(i) - v2(i);
    return a;
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne &operator-=(VectorTypeOne &v1,
                                          const VectorTypeTwo &v2) {
    for(int i = 0; i < VectorTypeOne::Size; i++) {
      v1(i) -= v2(i);
    }
    return v1;
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType operator*(const VectorType &v, const ScalarType &s) {
    VectorType a;
    for(int i = 0; i < VectorType::Size; i++)
      a(i) = v(i) * s;
    return a;
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType operator*(const ScalarType &s, const VectorType &v) {
    VectorType a;
    for(int i = 0; i < VectorType::Size; i++)
      a(i) = v(i) * s;
    return a;
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType operator/(const VectorType &v, const ScalarType &s) {
    VectorType a;
    for(int i = 0; i < VectorType::Size; i++)
      a(i) = v(i) / s;
    return a;
  }

  template<typename VectorType, typename ScalarType>
  static inline operator/(const ScalarType &s, const VectorType &v) {
    VectorType a;
    for(int i = 0; i < VectorType::Size; i++)
      a(i) = v(i) / s;
    return a;
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType &operator*=(VectorType &v, const ScalarType &s) {
    for(int i = 0; i < VectorType::Size; i++)
      v(i) *= s;
    return v;
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType &operator/=(VectorType &v, const ScalarType &s) {
    for(int i = 0; i < VectorType::Size; i++)
      v(i) /= s;
    return v;
  }
};

#endif  // BASE_INCLUDE_VECTORBASE_H_
