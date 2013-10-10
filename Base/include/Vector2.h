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

#ifndef BASE_INCLUDE_VECTOR2_H_
#define BASE_INCLUDE_VECTOR2_H_

#include "VectorBase.h"

#ifdef _VEX_ENABLE_SWIZZLE_
# define _VEX_VEC2_SWIZZLE_DEF(X, Y) \
    Vector2<T> X##Y() const { return Vector2<T>( X(), Y() ); }
#endif // _VEX_ENABLE_SWIZZLE_

namespace FasTC {

  template<typename T>
  class Vector2 : public VectorBase<T, 2> {
   public:
    // Ideally, we would be able to do this with initialization
    // lists, but I'm not really sure how to do that without gross
    // code duplication.
    Vector2() { }
    Vector2(T x, T y) {
      X() = x;
      Y() = y; 
    }

    // Overloaded functions
    template<typename _T>
    Vector2(const Vector2<_T> &v) : VectorBase<T, 2>(v) { }

    template<typename _T>
    Vector2<T> &operator=(const Vector2<_T> &v) {
      VectorBase<T, 2>::operator=(v);
      return *this;
    }

    // Accessors
    T &X() { return (*this)[0]; }
    const T &X() const { return (*this)[0]; }

    T &Y() { return (*this)[1]; }
    const T &Y() const { return (*this)[1]; }

    // Swizzle
    #ifdef _VEX_ENABLE_SWIZZLE_
    _VEX_VEC2_SWIZZLE_DEF(X, X)
    _VEX_VEC2_SWIZZLE_DEF(X, Y)
    _VEX_VEC2_SWIZZLE_DEF(Y, X)
    _VEX_VEC2_SWIZZLE_DEF(Y, Y)
    #endif //_VEX_ENABLE_SWIZZLE_
  };
  REGISTER_ONE_TEMPLATE_VECTOR_TYPE(Vector2);

  typedef Vector2<float> Vec2f;
  typedef Vector2<double> Vec2d;
  typedef Vector2<int> Vec2i;
};

#endif  // BASE_INCLUDE_VECTOR2_H_
