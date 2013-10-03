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

#ifndef BASE_INCLUDE_VECTOR3_H_
#define BASE_INCLUDE_VECTOR3_H_

#include "Vector2.h"

#ifdef _VEX_ENABLE_SWIZZLE_
# define _VEX_VEC3_SWIZZLE_DEF(X, Y, Z) \
    Vector3<T> X##Y##Z() const { return Vector3<T>( X(), Y(), Z() ); }
#endif // _VEX_ENABLE_SWIZZLE_

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
    
    // Overloaded functions
    template<typename _T>
    Vector3(const Vector3<_T> &v) : VectorBase<T, 3>(v) { }

    template<typename _T>
    Vector3<T> &operator=(const Vector3<_T> &v) {
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
    #ifdef _VEX_ENABLE_SWIZZLE_
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
    #endif // _VEX_ENABLE_SWIZZLE_
  };

  typedef Vector3<float> Vec3f;
  typedef Vector3<double> Vec3d;
  typedef Vector3<int> Vec3i;
};

#endif  // BASE_INCLUDE_VECTOR3_H_
