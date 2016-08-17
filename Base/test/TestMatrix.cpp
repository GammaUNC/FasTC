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

#include "gtest/gtest.h"
#include "FasTC/MatrixBase.h"

static const float kEpsilon = 1e-6f;

TEST(MatrixBase, Constructors) {
  FasTC::MatrixBase<float, 3, 4> m3f;
  for(int j = 0; j < 3; j++)
  for(int i = 0; i < 4; i++)
    m3f[j*4+i] = static_cast<float>(i) / static_cast<float>(j+1);

  FasTC::MatrixBase<double, 1, 1> m1d;
  for(int j = 0; j < 1; j++)
  for(int i = 0; i < 1; i++)
    m1d[j*1+i] = 0.0;

  FasTC::MatrixBase<int, 7, 200> m7i;
  for(int j = 0; j < 7; j++)
  for(int i = 0; i < 200; i++)
    m7i[j*200+i] = i*j;

  FasTC::MatrixBase<unsigned, 16, 16> m16u;
  for(int j = 0; j < 16; j++)
  for(int i = 0; i < 16; i++)
    m16u[j*16+i] = j-i;

#define TEST_VECTOR_COPY_CONS(mat, t, n, m)     \
  do {                                          \
    FasTC::MatrixBase<t, n, m> d##mat (mat);    \
    for(int i = 0; i < n*m; i++) {              \
      EXPECT_EQ(d##mat [i], mat[i]);            \
    }                                           \
  } while(0)                                    \

  TEST_VECTOR_COPY_CONS(m3f, float, 3, 4);
  TEST_VECTOR_COPY_CONS(m1d, double, 1, 1);
  TEST_VECTOR_COPY_CONS(m7i, int, 7, 200);
  TEST_VECTOR_COPY_CONS(m16u, unsigned, 16, 16);

#undef TEST_VECTOR_COPY_CONS
}

TEST(MatrixBase, Accessors) {
  FasTC::MatrixBase<float, 3, 1> v3f;
  v3f[0] = 1.0f;
  v3f[1] = -2.3f;
  v3f[2] = 1000;

  for(int i = 0; i < 3; i++) {
    EXPECT_EQ(v3f[i], v3f(i));
  }

  v3f(0) = -1.0f;
  v3f(1) = 2.3f;
  v3f(2) = -1000;

  for(int i = 0; i < 3; i++) {
    EXPECT_EQ(v3f(i), v3f[i]);
  }
}

TEST(MatrixBase, PointerConversion) {
  FasTC::MatrixBase<float, 2, 3> v3f;
  v3f(0,0) = 1.0f;
  v3f(0,1) = -2.3f;
  v3f(0,2) = 1000.0f;
  v3f(1,0) = 1.0f;
  v3f(1,1) = -2.3f;
  v3f(1,2) = 1000.0f;

  float cmp[6] = { 1.0f, -2.3f, 1000.0f, 1.0f, -2.3f, 1000.0f };
  const float *v3fp = v3f;
  int result = memcmp(cmp, v3fp, 6 * sizeof(float));
  EXPECT_EQ(result, 0);

  cmp[0] = -1.0f;
  cmp[1] = 2.3f;
  cmp[2] = 1000.0f;
  cmp[3] = -1.0f;
  cmp[4] = 2.3f;
  cmp[5] = 1000.0f;
  v3f = cmp;
  for(int i = 0; i < 3; i++) {
    EXPECT_EQ(v3f[i], cmp[i]);
  }
}

TEST(MatrixBase, CastVector) {
  srand(static_cast<unsigned>(time(NULL)));

  FasTC::MatrixBase<float, 3, 2> v3f;
  for(int i = 0; i < 6; i++) {
    v3f[i] = static_cast<float>(rand());
  }

  FasTC::MatrixBase<double, 3, 2> v3d = v3f;
  FasTC::MatrixBase<int, 3, 2> v3i = v3f;
  for(int i = 0; i < 3*2; i++) {
    EXPECT_EQ(v3d(i), static_cast<double>(v3f(i)));
    EXPECT_EQ(v3i(i), static_cast<int>(v3f(i)));
  }
}

TEST(MatrixBase, MatrixMultiplication) {
  FasTC::MatrixBase<int, 2, 3> a;
  a(0, 0) = 1; a(0, 1) = 2; a(0, 2) = 3;
  a(1, 0) = 4; a(1, 1) = 5; a(1, 2) = 6;

  FasTC::MatrixBase<int, 3, 5> b;
  b(0, 0) = -1;  b(0, 1) =  2;  b(0, 2) = -4; b(0, 3) =  5; b(0, 4) = 0; 
  b(1, 0) =  1;  b(1, 1) =  2;  b(1, 2) =  4; b(1, 3) =  6; b(1, 4) = 3; 
  b(2, 0) = -1;  b(2, 1) = -2;  b(2, 2) = -3; b(2, 3) = -4; b(2, 4) = 5; 

  FasTC::MatrixBase<float, 2, 5> amb = a * b;
  EXPECT_NEAR(amb(0, 0), -2, kEpsilon);
  EXPECT_NEAR(amb(0, 1), 0, kEpsilon);
  EXPECT_NEAR(amb(0, 2), -5, kEpsilon);
  EXPECT_NEAR(amb(0, 3), 5, kEpsilon);
  EXPECT_NEAR(amb(0, 4), 21, kEpsilon);

  EXPECT_NEAR(amb(1, 0), -5, kEpsilon);
  EXPECT_NEAR(amb(1, 1), 6, kEpsilon);
  EXPECT_NEAR(amb(1, 2), -14, kEpsilon);
  EXPECT_NEAR(amb(1, 3), 26, kEpsilon);
  EXPECT_NEAR(amb(1, 4), 45, kEpsilon);
}

TEST(MatrixBase, Transposition) {
  FasTC::MatrixBase<int, 3, 5> a;
  a(0, 0) = -1;  a(0, 1) =  2;  a(0, 2) = -4; a(0, 3) =  5; a(0, 4) = 0;
  a(1, 0) =  1;  a(1, 1) =  2;  a(1, 2) =  4; a(1, 3) =  6; a(1, 4) = 3;
  a(2, 0) = -1;  a(2, 1) = -2;  a(2, 2) = -3; a(2, 3) = -4; a(2, 4) = 5;

  FasTC::MatrixBase<int, 5, 3> b = a.Transpose();

  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 5; j++) {
      EXPECT_EQ(a(i, j), b(j, i));
    }
  }
}

TEST(MatrixBase, VectorMultiplication) {

  FasTC::MatrixBase<int, 3, 5> a;
  a(0, 0) = -1;  a(0, 1) =  2;  a(0, 2) = -4; a(0, 3) =  5; a(0, 4) = 0;
  a(1, 0) =  1;  a(1, 1) =  2;  a(1, 2) =  4; a(1, 3) =  6; a(1, 4) = 3;
  a(2, 0) = -1;  a(2, 1) = -2;  a(2, 2) = -3; a(2, 3) = -4; a(2, 4) = 5;

  FasTC::VectorBase<int, 5> v;
  for(int i = 0; i < 5; i++) v[i] = i + 1;

  FasTC::VectorBase<int, 3> u = a * v;
  EXPECT_EQ(u[0], -1 + (2 * 2) - (4 * 3) + (5 * 4));
  EXPECT_EQ(u[1], 1 + (2 * 2) + (4 * 3) + (6 * 4) + (3 * 5));
  EXPECT_EQ(u[2], -1 + (-2 * 2) - (3 * 3) - (4 * 4) + (5 * 5));

  /////

  for(int i = 0; i < 3; i++) u[i] = i + 1;
  v = u * a;

  EXPECT_EQ(v[0], -1 + (1 * 2) - (1 * 3));
  EXPECT_EQ(v[1], 2 + (2 * 2) - (2 * 3));
  EXPECT_EQ(v[2], -4 + (4 * 2) - (3 * 3));
  EXPECT_EQ(v[3], 5 + (6 * 2) - (4 * 3));
  EXPECT_EQ(v[4], 0 + (3 * 2) + (5 * 3));
}

////////////////////////////////////////////////////////////////////////////////

#include "FasTC/MatrixSquare.h"

TEST(MatrixSquare, Constructors) {
  FasTC::MatrixBase<int, 3, 3> m;
  m(0, 0) = 1; m(0, 1) = 2; m(0, 2) = 3;
  m(1, 0) = 2; m(1, 1) = 3; m(1, 2) = 4;
  m(2, 0) = 3; m(2, 1) = 4; m(2, 2) = 5;

  FasTC::MatrixSquare<int, 3> sqm (m);
  for(int i = 0; i < 9; i++) {
    EXPECT_EQ(m[i], sqm[i]);
  }

  FasTC::MatrixSquare<float, 3> fsqm(m);
  for(int i = 0; i < 9; i++) {
    EXPECT_NEAR(m[i], fsqm[i], kEpsilon);
  }

  FasTC::MatrixSquare<float, 3> fcsqm(sqm);
  for(int i = 0; i < 9; i++) {
    EXPECT_NEAR(fcsqm[i], sqm[i], kEpsilon);
  }
}

TEST(MatrixSquare, PowerMethod) {
  FasTC::MatrixSquare<double, 2> A;
  A(0, 0) = 0.8f; A(0, 1) = 0.3f;
  A(1, 0) = 0.2f; A(1, 1) = 0.7f;

  double e;
  FasTC::VectorBase<double, 2> x;
  A.PowerMethod(x, &e, 20);

  EXPECT_NEAR(x[0], 0.83205f, 0.0002);
  EXPECT_NEAR(x[1], 0.5547f, 0.0002);

  EXPECT_NEAR(e, 1.f, 0.0001);
}

////////////////////////////////////////////////////////////////////////////////

#include "FasTC/Matrix2x2.h"

TEST(Matrix2x2, Constructors) {
  FasTC::MatrixBase<int, 2, 2> m;
  m(0, 0) = 1; m(0, 1) = 2;
  m(1, 0) = 2; m(1, 1) = 3;

  FasTC::MatrixSquare<int, 2> sqm (m);
  for(int i = 0; i < 4; i++) {
    EXPECT_EQ(m[i], sqm[i]);
  }

  FasTC::Matrix2x2<int> tbtm (m);
  for(int i = 0; i < 4; i++) {
    EXPECT_EQ(m[i], tbtm[i]);
  }

  FasTC::Matrix2x2<float> fsqm(m);
  for(int i = 0; i < 4; i++) {
    EXPECT_NEAR(m[i], fsqm[i], kEpsilon);
  }

  FasTC::Matrix2x2<float> fcsqm(sqm);
  for(int i = 0; i < 4; i++) {
    EXPECT_NEAR(fcsqm[i], sqm[i], kEpsilon);
  }
}

////////////////////////////////////////////////////////////////////////////////

#include "FasTC/Matrix3x3.h"

TEST(Matrix3x3, Constructors) {
  FasTC::MatrixBase<int, 3, 3> m;
  m(0, 0) = 1; m(0, 1) = 2; m(0, 2) = 3;
  m(1, 0) = 2; m(1, 1) = 3; m(1, 2) = 4;
  m(2, 0) = 3; m(2, 1) = 4; m(2, 2) = 5;

  FasTC::MatrixSquare<int, 3> sqm (m);
  for(int i = 0; i < 9; i++) {
    EXPECT_EQ(m[i], sqm[i]);
  }

  FasTC::Matrix3x3<int> tbtm (m);
  for(int i = 0; i < 9; i++) {
    EXPECT_EQ(m[i], tbtm[i]);
  }

  FasTC::Matrix3x3<float> fsqm(m);
  for(int i = 0; i < 9; i++) {
    EXPECT_NEAR(m[i], fsqm[i], kEpsilon);
  }

  FasTC::Matrix3x3<float> fcsqm(sqm);
  for(int i = 0; i < 9; i++) {
    EXPECT_NEAR(fcsqm[i], sqm[i], kEpsilon);
  }
}

////////////////////////////////////////////////////////////////////////////////

#include "FasTC/Matrix4x4.h"

TEST(Matrix4x4, Constructors) {
  FasTC::MatrixBase<int, 4, 4> m;
  m(0, 0) = 1; m(0, 1) = 2; m(0, 2) = 3; m(0, 3) = 4;
  m(1, 0) = 2; m(1, 1) = 3; m(1, 2) = 4; m(1, 3) = 5;
  m(2, 0) = 3; m(2, 1) = 4; m(2, 2) = 5; m(2, 3) = 6;
  m(3, 0) = 4; m(3, 1) = 5; m(3, 2) = 6; m(3, 3) = 7;

  FasTC::MatrixSquare<int, 4> sqm (m);
  for(int i = 0; i < 9; i++) {
    EXPECT_EQ(m[i], sqm[i]);
  }

  FasTC::Matrix4x4<int> tbtm (m);
  for(int i = 0; i < 9; i++) {
    EXPECT_EQ(m[i], tbtm[i]);
  }

  FasTC::Matrix4x4<float> fsqm(m);
  for(int i = 0; i < 9; i++) {
    EXPECT_NEAR(m[i], fsqm[i], kEpsilon);
  }

  FasTC::Matrix4x4<float> fcsqm(sqm);
  for(int i = 0; i < 9; i++) {
    EXPECT_NEAR(fcsqm[i], sqm[i], kEpsilon);
  }
}

