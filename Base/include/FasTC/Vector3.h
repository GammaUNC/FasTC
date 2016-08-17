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

#ifndef BASE_INCLUDE_VECTOR3_H_
#define BASE_INCLUDE_VECTOR3_H_

#include "Vector2.h"

# define _VEX_VEC3_SWIZZLE_DEF(X, Y, Z) \
    Vector3<T> X##Y##Z() const { return Vector3<T>( X(), Y(), Z() ); }

namespace FasTC {

  template <typename T>
  class Vector3 : public VectorBase<T, 3> {
   public: 
    Vector3() { }
    Vector3(T x, T y, T z) {
      X() = x;
      Y() = y;
      Z() = z;
    }

    explicit Vector3(const T *_vec) {
      for(int i = 0; i < 3; i++) {
        this->vec[i] = _vec[i];
      }
    }
    
    // Overloaded functions
    template<typename _T>
    Vector3(const Vector3<_T> &v) : VectorBase<T, 3>(v) { }

    template<typename _T>
    Vector3<T> &operator=(const Vector3<_T> &v) {
      VectorBase<T, 3>::operator=(v);
      return *this;
    }

    template<typename _T>
    Vector3<T> &operator=(const _T *v) {
      VectorBase<T, 3>::operator=(v);
      return *this;
    }

    // Accessors
    T &X() { return (*this)[0]; }
    const T &X() const { return (*this)[0]; }

    T &Y() { return (*this)[1]; }
    const T &Y() const { return (*this)[1]; }

    T &Z() { return (*this)[2]; }
    const T &Z() const { return (*this)[2]; }

    // Vector operations
    template<typename _T>
    Vector3<T> Cross(const Vector3<_T> &v) {
      return Vector3<T>(
        Y() * v.Z() - v.Y() * Z(),
        Z() * v.X() - v.Z() * X(),
        X() * v.Y() - v.X() * Y()
      );
    }

    // Swizzle
    _VEX_VEC2_SWIZZLE_DEF(X, X)
    _VEX_VEC2_SWIZZLE_DEF(X, Y)
    _VEX_VEC2_SWIZZLE_DEF(X, Z)
    _VEX_VEC2_SWIZZLE_DEF(Y, X)
    _VEX_VEC2_SWIZZLE_DEF(Y, Y)
    _VEX_VEC2_SWIZZLE_DEF(Y, Z)
    _VEX_VEC2_SWIZZLE_DEF(Z, X)
    _VEX_VEC2_SWIZZLE_DEF(Z, Y)
    _VEX_VEC2_SWIZZLE_DEF(Z, Z)

    _VEX_VEC3_SWIZZLE_DEF(X, X, X)
    _VEX_VEC3_SWIZZLE_DEF(X, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, Z)
  };
  REGISTER_ONE_TEMPLATE_VECTOR_TYPE(Vector3);

  typedef Vector3<float> Vec3f;
  typedef Vector3<double> Vec3d;
  typedef Vector3<int> Vec3i;
};

#endif  // BASE_INCLUDE_VECTOR3_H_
