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

#ifndef BASE_INCLUDE_VECTORBASE_H_
#define BASE_INCLUDE_VECTORBASE_H_

// !FIXME! For sqrt function. This increases compilation time by a LOT 
// but I couldn't guarantee any faster general-purpose implementation
#include <cmath>

namespace FasTC {

  enum EVectorType {
    eVectorType_Scalar,
    eVectorType_Vector,
    eVectorType_Matrix
  };

  template <typename T, const int N>
  class VectorBase {
   protected:

    // Vector representation
    T vec[N];

   public:
    typedef T ScalarType;
    
    VectorBase() { }
    VectorBase(const VectorBase<T, N> &other) {
      for(int i = 0; i < N; i++) vec[i] = other[i];
    }

    explicit VectorBase(const T *_vec) {
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
    operator const T *() const { return vec; }
    VectorBase<T, N> &operator=(const T *v) {
      for(int i = 0; i < N; i++)
        vec[i] = v[i];
      return *this;
    }

    // Equality comparison
    template<typename _T>
    bool operator==(const VectorBase<_T, N> &v) const {
      bool result = true;
      for(int i = 0; i < N; i++)
        result = result && (vec[i] == v[i]);
      return result;
    }

    template<typename _T>
    bool operator!=(const VectorBase<_T, N> &v) const {
      return !(operator==(v));
    }

    // Allows casting to other vector types if the underlying type system does as well...
    template<typename _T>
    operator VectorBase<_T, N>() const { 
      VectorBase<_T, N> ret;
      for(int i = 0; i < N; i++) {
        ret[i] = static_cast<_T>(vec[i]);
      }
      return ret;
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
    T Length() const { return static_cast<T>(
      sqrt(static_cast<long double>(LengthSq())));}

    void Normalize() {
      T len = Length();
      for(int i = 0; i < N; i++) {
        vec[i] /= len;
      }
    }
  };

  // Operators
  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne VectorAddition(const VectorTypeOne &v1,
                                             const VectorTypeTwo &v2) {
    VectorTypeOne a(v1);
    for(int i = 0; i < VectorTypeOne::Size; i++) {
      a(i) += static_cast<typename VectorTypeOne::ScalarType>(v2[i]);
    }
    return a;
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne operator+(const VectorTypeOne &v1,
                                        const VectorTypeTwo &v2) {
    return VectorAddition(v1, v2);
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne &operator+=(VectorTypeOne &v1,
                                          const VectorTypeTwo &v2) {
    return v1 = VectorAddition(v1, v2);
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne VectorSubtraction(const VectorTypeOne &v1,
                                                const VectorTypeTwo &v2) {
    VectorTypeOne a(v1);
    for(int i = 0; i < VectorTypeOne::Size; i++) {
      a(i) -= static_cast<typename VectorTypeOne::ScalarType>(v2[i]);
    }
    return a;
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne operator-(const VectorTypeOne &v1,
                                        const VectorTypeTwo &v2) {
    return VectorSubtraction(v1, v2);
  }

  template<typename VectorTypeOne, typename VectorTypeTwo>
  static inline VectorTypeOne &operator-=(VectorTypeOne &v1,
                                          const VectorTypeTwo &v2) {
    return v1 = VectorSubtraction(v1, v2);
  }

  template<typename T>
  class VectorTraits {
   public:
    static const EVectorType kVectorType = eVectorType_Scalar;
  };

  template<typename T, const int N>
  class VectorTraits<VectorBase<T, N> > {
   public:
    static const EVectorType kVectorType = eVectorType_Vector;
  };

  #define REGISTER_VECTOR_TYPE(TYPE)                           \
  template<>                                                   \
  class VectorTraits< TYPE > {                                 \
  public:                                                      \
    static const EVectorType kVectorType = eVectorType_Vector; \
  }

  #define REGISTER_ONE_TEMPLATE_VECTOR_TYPE(TYPE)              \
  template<typename T>                                         \
  class VectorTraits< TYPE <T> > {                             \
  public:                                                      \
    static const EVectorType kVectorType = eVectorType_Vector; \
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType ScalarMultiply(const VectorType &v, const ScalarType &s) {
    VectorType a(v);
    for(int i = 0; i < VectorType::Size; i++)
      a(i) = static_cast<typename VectorType::ScalarType>(a(i) * s);
    return a;
  }

  // !WTF! MSVC bug with enums in template parameters =(
  template<
    /* EVectorType */unsigned kVectorTypeOne,
    /* EVectorType */unsigned kVectorTypeTwo,
    typename TypeOne,
    typename TypeTwo>
  class MultSwitch {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;
   public:
    typedef TypeOne ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return m_A * m_B; }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Scalar,
    eVectorType_Vector,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef TypeTwo ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return ScalarMultiply(m_B, m_A); }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Vector,
    eVectorType_Scalar,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef TypeOne ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return ScalarMultiply(m_A, m_B); }
  };

  template<typename TypeOne, typename TypeTwo>
  class MultSwitch<
    eVectorType_Vector,
    eVectorType_Vector,
    TypeOne, TypeTwo> {
   private:
    const TypeOne &m_A;
    const TypeTwo &m_B;

   public:
    typedef typename TypeOne::ScalarType ResultType;

    MultSwitch(const TypeOne &a, const TypeTwo &b)
      : m_A(a), m_B(b) { }

    ResultType GetMultiplication() const { return m_A.Dot(m_B); }
  };

  template<typename TypeOne, typename TypeTwo>
  static inline 
    typename MultSwitch<
      VectorTraits<TypeOne>::kVectorType,
      VectorTraits<TypeTwo>::kVectorType,
      TypeOne, TypeTwo
    >::ResultType
  operator*(const TypeOne &v1, const TypeTwo &v2) {
    typedef MultSwitch<
      VectorTraits<TypeOne>::kVectorType,
      VectorTraits<TypeTwo>::kVectorType,
      TypeOne, TypeTwo
    > VSwitch;
    return VSwitch(v1, v2).GetMultiplication();
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType &operator*=(VectorType &v, const ScalarType &s) {
    return v = v * s;
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType ScalarDivide(const VectorType &v, const ScalarType &s) {
    VectorType a(v);
    for(int i = 0; i < VectorType::Size; i++)
      a(i) = static_cast<typename VectorType::ScalarType>(a(i) / s);
    return a;
  }

  template<typename TypeOne, typename TypeTwo>
  static inline TypeOne operator/(const TypeOne &v1, const TypeTwo &v2) {
    return ScalarDivide(v1, v2);
  }

  template<typename VectorType, typename ScalarType>
  static inline VectorType &operator/=(VectorType &v, const ScalarType &s) {
    return v = ScalarDivide(v, s);
  }
};

#endif  // BASE_INCLUDE_VECTORBASE_H_
