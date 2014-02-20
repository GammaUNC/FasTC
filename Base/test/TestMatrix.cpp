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
  FasTC::MatrixBase<float, 3, 2> v3f;
  FasTC::MatrixBase<double, 3, 2> v3d = v3f;
  FasTC::MatrixBase<int, 3, 2> v3i = v3f;
  for(int i = 0; i < 3*2; i++) {
    EXPECT_EQ(v3d(i), static_cast<double>(v3f(i)));
    EXPECT_EQ(v3i(i), static_cast<int>(v3f(i)));
  }
}

TEST(MatrixBase, MatrixMultiplication) {
  // Stub
  EXPECT_EQ(0, 1);
}

TEST(MatrixBase, VectorMultiplication) {
  // Stub
  EXPECT_EQ(0, 1);
}

TEST(MatrixSquare, Constructors) {
  // Stub
  EXPECT_EQ(0, 1);
}

TEST(MatrixSquare, EigenvalueCalculation) {
  // Stub
  EXPECT_EQ(0, 1);
}

TEST(Matrix2x2, Constructors) {
  // Stub
  EXPECT_EQ(0, 1);
}

TEST(Matrix3x3, Constructors) {
  // Stub
  EXPECT_EQ(0, 1);
}

TEST(Matrix4x4, Constructors) {
  // Stub
  EXPECT_EQ(0, 1);
}

