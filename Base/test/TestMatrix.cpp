/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#include "gtest/gtest.h"
#include "MatrixBase.h"

static const float kEpsilon = 1e-6;

TEST(MatrixBase, Constructors) {
  FasTC::MatrixBase<float, 3, 4> m3f;
  FasTC::MatrixBase<double, 1, 1> m1d;
  FasTC::MatrixBase<int, 7, 200> m7i;
  FasTC::MatrixBase<unsigned, 16, 16> m16u;

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
  srand(time(NULL));

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

#include "MatrixSquare.h"

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
  A.PowerMethod(x, &e, 20, 200);

  EXPECT_NEAR(x[0], 0.83205f, 0.0002);
  EXPECT_NEAR(x[1], 0.5547f, 0.0002);

  EXPECT_NEAR(e, 1.f, 0.0001);
}

////////////////////////////////////////////////////////////////////////////////

#include "Matrix2x2.h"

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

#include "Matrix3x3.h"

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

#include "Matrix4x4.h"

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

