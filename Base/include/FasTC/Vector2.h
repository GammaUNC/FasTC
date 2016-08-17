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

#ifndef BASE_INCLUDE_VECTOR2_H_
#define BASE_INCLUDE_VECTOR2_H_

#include "VectorBase.h"

# define _VEX_VEC2_SWIZZLE_DEF(X, Y) \
    Vector2<T> X##Y() const { return Vector2<T>( X(), Y() ); }

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

    explicit Vector2(const T *_vec) {
      for(int i = 0; i < 2; i++)
        this->vec[i] = _vec[i];
    }

    // Overloaded functions
    template<typename _T>
    Vector2(const Vector2<_T> &v) : VectorBase<T, 2>(v) { }

    template<typename _T>
    Vector2<T> &operator=(const Vector2<_T> &v) {
      VectorBase<T, 2>::operator=(v);
      return *this;
    }

    Vector2<T> &operator=(const T *_vec) {
      VectorBase<T, 2>::operator=(_vec);
      return *this;
    }

    // Accessors
    T &X() { return (*this)[0]; }
    const T &X() const { return (*this)[0]; }

    T &Y() { return (*this)[1]; }
    const T &Y() const { return (*this)[1]; }

    // Swizzle
    _VEX_VEC2_SWIZZLE_DEF(X, X)
    _VEX_VEC2_SWIZZLE_DEF(X, Y)
    _VEX_VEC2_SWIZZLE_DEF(Y, X)
    _VEX_VEC2_SWIZZLE_DEF(Y, Y)
  };
  REGISTER_ONE_TEMPLATE_VECTOR_TYPE(Vector2);

  typedef Vector2<float> Vec2f;
  typedef Vector2<double> Vec2d;
  typedef Vector2<int> Vec2i;
};

#endif  // BASE_INCLUDE_VECTOR2_H_
