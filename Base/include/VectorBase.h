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

    // Operators
    template<typename _T>
    VectorBase<T, N> operator+(const VectorBase<_T, N> &v) const {
      VectorBase a;
      for(int i = 0; i < N; i++)
        a.vec[i] = v(i) + vec[i];
      return a;
    }

    template<typename _T>
    VectorBase<T, N> &operator+=(const VectorBase<_T, N> &v) const {
      for(int i = 0; i < N; i++)
        vec[i] += v(i);
      return *this;
    }

    template<typename _T>
    VectorBase<T, N> operator-(const VectorBase<_T, N> &v) const {
      VectorBase<T, N> a;
      for(int i = 0; i < N; i++)
        a(i) = vec[i] - v[i];
      return a;
    }

    template<typename _T>
    VectorBase<T, N> &operator-=(const VectorBase<_T, N> &v) const {
      for(int i = 0; i < N; i++) {
        vec[i] -= v[i];
      }
      return *this;
    }

    template<typename _T>
    VectorBase<T, N> &operator=(const VectorBase<_T, N> &v) {
      for(int i = 0; i < N; i++)
        vec[i] = v[i];
      return *this;
    }

    template<typename _T>
    VectorBase<T, N> operator*(const _T s) const {
      VectorBase<T, N> a;
      for(int i = 0; i < N; i++)
        a[i] = vec[i] * s;
      return a;
    }
  
    template<typename _T>
    friend VectorBase<T, N> operator*(const _T s, const VectorBase<T, N> &v) {
      VectorBase<T, N> a;
      for(int i = 0; i < N; i++)
        a[i] = v[i] * s;
      return a;
    }

    template<typename _T>
    VectorBase<T, N> operator/(const _T s) const {
      VectorBase<T, N> a;
      for(int i = 0; i < N; i++)
        a[i] = vec[i] / s;
      return a;
    }
  
    template<typename _T>
    friend VectorBase<T, N> operator/(const _T s, const VectorBase<T, N> &v) {
      VectorBase<T, N> a;
      for(int i = 0; i < N; i++)
        a[i] = v[i] / s;
      return a;
    }

    template<typename _T>
    void operator*=(const _T s) {
      for(int i = 0; i < N; i++)
        vec[i] *= s;
    }

    template<typename _T>
    void operator/=(const _T s) {
      for(int i = 0; i < N; i++)
        vec[i] /= s;
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
};

#endif  // BASE_INCLUDE_VECTORBASE_H_
