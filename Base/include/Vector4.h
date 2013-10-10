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

#ifndef BASE_INCLUDE_VECTOR4_H_
#define BASE_INCLUDE_VECTOR4_H_

#include "TexCompTypes.h"
#include "Vector3.h"

#ifdef _VEX_ENABLE_SWIZZLE_
#define _VEX_VEC4_SWIZZLE_DEF(X, Y, Z, W) \
  Vector4<T> X##Y##Z##W() const { return Vector4<T>( X(), Y(), Z(), W() ); }
#endif // _VEX_ENABLE_SWIZZLE_

namespace FasTC {

  template <typename T>
  class Vector4 : public VectorBase<T, 4> {
   public: 
    Vector4() { }
    Vector4(T x, T y, T z, T w) {
      X() = x;
      Y() = y;
      Z() = z;
      W() = w;
    }
    
    // Overloaded functions
    template<typename _T>
    Vector4(const Vector4<_T> &v) : VectorBase<T, 4>(v) { }

    template<typename _T>
    Vector4<T> &operator=(const Vector4<_T> &v) {
      VectorBase<T, 4>::operator=(v);
      return *this;
    }

    // Accessors
    T &X() { return (*this)[0]; }
    const T &X() const { return (*this)[0]; }

    T &Y() { return (*this)[1]; }
    const T &Y() const { return (*this)[1]; }

    T &Z() { return (*this)[2]; }
    const T &Z() const { return (*this)[2]; }

    T &W() { return (*this)[3]; }
    const T &W() const { return (*this)[3]; }

    // Swizzle
    #ifdef _VEX_ENABLE_SWIZZLE_
    _VEX_VEC2_SWIZZLE_DEF(X, X)
    _VEX_VEC2_SWIZZLE_DEF(X, Y)
    _VEX_VEC2_SWIZZLE_DEF(X, Z)
    _VEX_VEC2_SWIZZLE_DEF(X, W)
    _VEX_VEC2_SWIZZLE_DEF(Y, X)
    _VEX_VEC2_SWIZZLE_DEF(Y, Y)
    _VEX_VEC2_SWIZZLE_DEF(Y, Z)
    _VEX_VEC2_SWIZZLE_DEF(Y, W)
    _VEX_VEC2_SWIZZLE_DEF(Z, X)
    _VEX_VEC2_SWIZZLE_DEF(Z, Y)
    _VEX_VEC2_SWIZZLE_DEF(Z, Z)
    _VEX_VEC2_SWIZZLE_DEF(Z, W)
    _VEX_VEC2_SWIZZLE_DEF(W, X)
    _VEX_VEC2_SWIZZLE_DEF(W, Y)
    _VEX_VEC2_SWIZZLE_DEF(W, Z)
    _VEX_VEC2_SWIZZLE_DEF(W, W)

    _VEX_VEC3_SWIZZLE_DEF(X, X, X)
    _VEX_VEC3_SWIZZLE_DEF(X, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(X, X, W)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(X, Y, W)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, Z)
    _VEX_VEC3_SWIZZLE_DEF(X, Z, W)
    _VEX_VEC3_SWIZZLE_DEF(X, W, X)
    _VEX_VEC3_SWIZZLE_DEF(X, W, Y)
    _VEX_VEC3_SWIZZLE_DEF(X, W, Z)
    _VEX_VEC3_SWIZZLE_DEF(X, W, W)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, X, W)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, Y, W)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, Z, W)
    _VEX_VEC3_SWIZZLE_DEF(Y, W, X)
    _VEX_VEC3_SWIZZLE_DEF(Y, W, Y)
    _VEX_VEC3_SWIZZLE_DEF(Y, W, Z)
    _VEX_VEC3_SWIZZLE_DEF(Y, W, W)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, X, W)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, Y, W)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, Z, W)
    _VEX_VEC3_SWIZZLE_DEF(Z, W, X)
    _VEX_VEC3_SWIZZLE_DEF(Z, W, Y)
    _VEX_VEC3_SWIZZLE_DEF(Z, W, Z)
    _VEX_VEC3_SWIZZLE_DEF(Z, W, W)
    _VEX_VEC3_SWIZZLE_DEF(W, X, X)
    _VEX_VEC3_SWIZZLE_DEF(W, X, Y)
    _VEX_VEC3_SWIZZLE_DEF(W, X, Z)
    _VEX_VEC3_SWIZZLE_DEF(W, X, W)
    _VEX_VEC3_SWIZZLE_DEF(W, Y, X)
    _VEX_VEC3_SWIZZLE_DEF(W, Y, Y)
    _VEX_VEC3_SWIZZLE_DEF(W, Y, Z)
    _VEX_VEC3_SWIZZLE_DEF(W, Y, W)
    _VEX_VEC3_SWIZZLE_DEF(W, Z, X)
    _VEX_VEC3_SWIZZLE_DEF(W, Z, Y)
    _VEX_VEC3_SWIZZLE_DEF(W, Z, Z)
    _VEX_VEC3_SWIZZLE_DEF(W, Z, W)
    _VEX_VEC3_SWIZZLE_DEF(W, W, X)
    _VEX_VEC3_SWIZZLE_DEF(W, W, Y)
    _VEX_VEC3_SWIZZLE_DEF(W, W, Z)
    _VEX_VEC3_SWIZZLE_DEF(W, W, W)

    _VEX_VEC4_SWIZZLE_DEF(X, X, X, X)
    _VEX_VEC4_SWIZZLE_DEF(X, X, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, X, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, X, X, W)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, X, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(X, X, W, X)
    _VEX_VEC4_SWIZZLE_DEF(X, X, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, X, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, X, W, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, X, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, X, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, W, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Y, W, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, X, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, X, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, W, X)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, Z, W, W)
    _VEX_VEC4_SWIZZLE_DEF(X, W, X, X)
    _VEX_VEC4_SWIZZLE_DEF(X, W, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, W, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, W, X, W)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, W, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(X, W, W, X)
    _VEX_VEC4_SWIZZLE_DEF(X, W, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(X, W, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(X, W, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, X, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Y, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, Z, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Y, W, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, X, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Y, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, Z, W, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, X, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, X, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, W, X)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(Z, W, W, W)
    _VEX_VEC4_SWIZZLE_DEF(W, X, X, X)
    _VEX_VEC4_SWIZZLE_DEF(W, X, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, X, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, X, X, W)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, X, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(W, X, W, X)
    _VEX_VEC4_SWIZZLE_DEF(W, X, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, X, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, X, W, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, X, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, X, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, W, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Y, W, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, X, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, X, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, W, X)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, Z, W, W)
    _VEX_VEC4_SWIZZLE_DEF(W, W, X, X)
    _VEX_VEC4_SWIZZLE_DEF(W, W, X, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, W, X, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, W, X, W)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Y, X)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Y, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Y, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Y, W)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Z, X)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Z, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Z, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, W, Z, W)
    _VEX_VEC4_SWIZZLE_DEF(W, W, W, X)
    _VEX_VEC4_SWIZZLE_DEF(W, W, W, Y)
    _VEX_VEC4_SWIZZLE_DEF(W, W, W, Z)
    _VEX_VEC4_SWIZZLE_DEF(W, W, W, W)
    #endif // _VEX_ENABLE_SWIZZLE_
  };
  REGISTER_ONE_TEMPLATE_VECTOR_TYPE(Vector4);

  typedef Vector4<float> Vec4f;
  typedef Vector4<double> Vec4d;
  typedef Vector4<uint32> Vec4i;
};

#endif  // BASE_INCLUDE_VECTOR4_H_
