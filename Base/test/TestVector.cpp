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
#include "FasTC/VectorBase.h"

static const float kEpsilon = 1e-6f;

TEST(VectorBase, Constructors) {
  FasTC::VectorBase<float, 3> v3f;
  v3f[0] = 1.1f; v3f[1] = 1.2f; v3f[2] = 1.3f;
  FasTC::VectorBase<double, 1> v1d;
  v1d[0] = 1.1;
  FasTC::VectorBase<int, 7> v7i;
  for(int i = 0; i < 7; i++)
	v7i[i] = -i;
  FasTC::VectorBase<unsigned, 16> v16u;
  for(int i = 0; i < 16; i++)
	v16u[i] = i;

#define TEST_VECTOR_COPY_CONS(v, t, n)          \
  do {                                          \
    FasTC::VectorBase<t, n> d##v (v);           \
    for(int i = 0; i < n; i++) {                \
      EXPECT_EQ(d##v [i], v[i]);                \
    }                                           \
  } while(0)                                    \

  TEST_VECTOR_COPY_CONS(v3f, float, 3);
  TEST_VECTOR_COPY_CONS(v1d, double, 1);
  TEST_VECTOR_COPY_CONS(v7i, int, 7);
  TEST_VECTOR_COPY_CONS(v16u, unsigned, 16);

#undef TEST_VECTOR_COPY_CONS
}

TEST(VectorBase, Accessors) {
  FasTC::VectorBase<float, 3> v3f;
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

TEST(VectorBase, PointerConversion) {
  FasTC::VectorBase<float, 3> v3f;
  v3f[0] = 1.0f;
  v3f[1] = -2.3f;
  v3f[2] = 1000;

  float cmp[3] = { 1.0f, -2.3f, 1000 };
  const float *v3fp = v3f;
  int result = memcmp(cmp, v3fp, 3 * sizeof(float));
  EXPECT_EQ(result, 0);

  cmp[0] = -1.0f;
  cmp[1] = 2.3f;
  cmp[2] = 1000.0f;
  v3f = cmp;
  for(int i = 0; i < 3; i++) {
    EXPECT_EQ(v3f[i], cmp[i]);
  }
}

TEST(VectorBase, CastVector) {
  FasTC::VectorBase<float, 3> v3f;
  v3f[0] = 1000000.0f;
  v3f[1] = -2.0f;
  v3f[2] = -1.1f;

  FasTC::VectorBase<double, 3> v3d = v3f;
  FasTC::VectorBase<int, 3> v3i = v3f;
  for(int i = 0; i < 3; i++) {
    EXPECT_EQ(v3d(i), static_cast<double>(v3f(i)));
    EXPECT_EQ(v3i(i), static_cast<int>(v3f(i)));
  }
}

TEST(VectorBase, DotProduct) {
  int iv[5] = { -2, -1, 0, 1, 2 };
  FasTC::VectorBase<int, 5> v5i(iv);

  unsigned uv[5] = { 1, 2, 3, 4, 5 };
  FasTC::VectorBase<unsigned, 5> v5u(uv);

  EXPECT_EQ(v5i.Dot(v5u), static_cast<int>(10));
  EXPECT_EQ(v5u.Dot(v5i), static_cast<unsigned>(10));

  EXPECT_EQ(v5i * v5u, static_cast<int>(10));
  EXPECT_EQ(v5u * v5i, static_cast<unsigned>(10));
}

TEST(VectorBase, Length) {
  int iv[5] = { 1, 2, 3, 4, 5 };
  FasTC::VectorBase<int, 5> v5i (iv);

  EXPECT_EQ(v5i.LengthSq(), 55);
  EXPECT_EQ(v5i.Length(), 7);

  float fv[6] = {1, 2, 3, 4, 5, 6};
  FasTC::VectorBase<float, 6> v6f (fv);
  
  EXPECT_EQ(v6f.LengthSq(), 91);
  EXPECT_NEAR(v6f.Length(), sqrt(91.0f), kEpsilon);
}

TEST(VectorBase, Normalization) {
  float fv[2] = {1, 0};
  FasTC::VectorBase<float, 2> v2f (fv);
  v2f.Normalize();
  EXPECT_NEAR(v2f[0], 1, kEpsilon);
  EXPECT_NEAR(v2f[1], 0, kEpsilon);

  // Normalized vector should be sqrt(2) for each axis, although
  // this can't be represented as integers...
  unsigned uv[2] = {2, 2};
  FasTC::VectorBase<unsigned, 2> v2u (uv);
  v2u.Normalize();
  EXPECT_EQ(v2u[0], static_cast<unsigned>(1));
  EXPECT_EQ(v2u[1], static_cast<unsigned>(1));

  const double sqrt2 = sqrt(2.0f)/2.0f;
  for(int i = 2; i < 10; i++) {
    v2f[0] = static_cast<float>(i);
    v2f[1] = static_cast<float>(i);
    v2f.Normalize();
    EXPECT_NEAR(v2f[0], sqrt2, kEpsilon);
    EXPECT_NEAR(v2f[1], sqrt2, kEpsilon);
  }
}

TEST(VectorBase, Scaling) {
  float fv[2] = {1.0f, 3.0f};
  FasTC::VectorBase<float, 2> v2f (fv);
  FasTC::VectorBase<float, 2> v2fd = v2f * 3.0f;
  EXPECT_NEAR(v2fd[0], 3.0f, kEpsilon);
  EXPECT_NEAR(v2fd[1], 9.0f, kEpsilon);

  v2fd = -1.0 * v2f;
  EXPECT_NEAR(v2fd[0], -1.0f, kEpsilon);
  EXPECT_NEAR(v2fd[1], -3.0f, kEpsilon);

  v2fd = v2f / 3;
  EXPECT_NEAR(v2fd[0], 1.0f / 3.0f, kEpsilon);
  EXPECT_NEAR(v2fd[1], 1.0f, kEpsilon);

  unsigned uv[2] = {1, 3};
  FasTC::VectorBase<unsigned, 2> v2u (uv);
  FasTC::VectorBase<unsigned, 2> v2ud = v2u * 0.5;
  EXPECT_EQ(v2ud[0], static_cast<unsigned>(0));
  EXPECT_EQ(v2ud[1], static_cast<unsigned>(1));

  v2ud = v2u / 0.5f;
  EXPECT_EQ(v2ud[0], static_cast<unsigned>(2));
  EXPECT_EQ(v2ud[1], static_cast<unsigned>(6));  
}

TEST(VectorBase, Addition) {
  float fv[2] = {1.1f, 3.2f};
  FasTC::VectorBase<float, 2> v2f (fv);

  int uv[2] = {5, 2};
  FasTC::VectorBase<int, 2> v2u (uv);

  FasTC::VectorBase<int, 2> au = v2u + v2f;
  EXPECT_EQ(au[0], 6);
  EXPECT_EQ(au[1], 5);

  au = v2u + fv + uv;
  EXPECT_EQ(au[0], 11);
  EXPECT_EQ(au[1], 7);

  FasTC::VectorBase<float, 2> af = v2f + v2u;
  EXPECT_NEAR(af[0], 6.1f, kEpsilon);
  EXPECT_NEAR(af[1], 5.2f, kEpsilon);

  au = v2u - v2f;
  EXPECT_EQ(au[0], 4);
  EXPECT_EQ(au[1], -1);

  af = v2f - v2u;
  EXPECT_NEAR(af[0], -3.9f, kEpsilon);
  EXPECT_NEAR(af[1], 1.2f, kEpsilon);  
}

////////////////////////////////////////////////////////////////////////////////
//
// Vec2
//
////////////////////////////////////////////////////////////////////////////////

#include "FasTC/Vector2.h"

TEST(Vector2, BaseFunctionality) {
  FasTC::Vec2f v2f;
  FasTC::Vec2d v2d;
  v2d.X() = 3.0;
  v2d.Y() = -10.0;

  v2f = v2d;
  EXPECT_EQ(v2f[0], v2d[0]);
  EXPECT_EQ(v2f[1], v2d[1]);
}

TEST(Vector2, Accessors) {
  float fv[2] = { 1.0f, 2.0f };
  FasTC::Vec2f v2f (fv);
  EXPECT_EQ(v2f.X(), 1.0f);
  EXPECT_EQ(v2f.Y(), 2.0f);

  v2f.X() = 4.0f;
  v2f.Y() = 5.0f;

  EXPECT_EQ(v2f.X(), 4.0f);
  EXPECT_EQ(v2f.Y(), 5.0f);
}

TEST(Vector2, Addition) {
  float fv[2] = { 1.0f, 2.0f };
  FasTC::Vec2f v2f (fv);

  double dv[2] = { 4.3, -10.2 };
  FasTC::Vec2d v2d (dv);

  EXPECT_NEAR((v2f + v2d).X(), 5.3, kEpsilon);
  EXPECT_NEAR((v2f + v2d).Y(), -8.2, kEpsilon);
}

TEST(Vector2, Swizzle) {
  float fv[2] = {1.0f, 2.0f};
  FasTC::Vec2f v;
  v = fv;

  EXPECT_EQ(v.XX().X(), 1.0f);
  EXPECT_EQ(v.XX().Y(), 1.0f);
  EXPECT_EQ(v.YY().X(), 2.0f);
  EXPECT_EQ(v.YY().Y(), 2.0f);
  EXPECT_EQ(v.YX().X(), 2.0f);
  EXPECT_EQ(v.YX().Y(), 1.0f);
  EXPECT_EQ(v.XY().X(), 1.0f);
  EXPECT_EQ(v.XY().Y(), 2.0f);
}

////////////////////////////////////////////////////////////////////////////////
//
// Vec3
//
////////////////////////////////////////////////////////////////////////////////

#include "FasTC/Vector3.h"

TEST(Vector3, BaseFunctionality) {
  FasTC::Vec3f vf;
  FasTC::Vec3d vd;
  vd.X() = 3.0;
  vd.Y() = -10.0;
  vd.Z() = 0.0;

  vf = vd;
  for(int i = 0; i < 3; i++) {
    EXPECT_EQ(vf[i], vd[i]);
  }
}

TEST(Vector3, Accessors) {
  float fv[3] = { 1.0f, 2.0f, 3.0f };
  FasTC::Vec3f v3f (fv);
  EXPECT_EQ(v3f.X(), 1.0f);
  EXPECT_EQ(v3f.Y(), 2.0f);
  EXPECT_EQ(v3f.Z(), 3.0f);

  v3f.X() = 4.0f;
  v3f.Y() = 5.0f;
  v3f.Z() = 6.0f;

  EXPECT_EQ(v3f.X(), 4.0f);
  EXPECT_EQ(v3f.Y(), 5.0f);
  EXPECT_EQ(v3f.Z(), 6.0f);
}

TEST(Vector3, Addition) {
  float fv[3] = { 1.0f, 2.0f, 3.0f };
  FasTC::Vec3f v3f (fv);

  double dv[3] = { 4.3, -10.2, 0.0f };
  FasTC::Vec3d v3d (dv);

  EXPECT_NEAR((v3f + v3d).X(), 5.3, kEpsilon);
  EXPECT_NEAR((v3f + v3d).Y(), -8.2, kEpsilon);
  EXPECT_NEAR((v3f + v3d).Z(), 3.0, kEpsilon);
}

TEST(Vector3, Swizzle) {
  float fv[3] = {1.0f, 2.0f, 3.0f};
  FasTC::Vec3f v;
  v = fv;

  EXPECT_EQ(v.XXX().Y(), 1.0f);
  EXPECT_EQ(v.YZX().X(), 2.0f);
  EXPECT_EQ(v.ZZY().Z(), 2.0f);
  EXPECT_EQ(v.ZYZ().X(), 3.0f);
}

TEST(Vector3, CrossProduct) {
  float fv[3] = {1.0f, 2.0f, 3.0f};
  FasTC::Vec3f v1 (fv);
  FasTC::Vec3f v2 = v1;
  std::swap(v1.X(), v1.Z());
  
  // Right handed coordinate system...
  FasTC::Vec3f r = v1.Cross(v2);
  EXPECT_NEAR(r.X(), 4.0f, kEpsilon);
  EXPECT_NEAR(r.Y(), -8.0f, kEpsilon);
  EXPECT_NEAR(r.Z(), 4.0f, kEpsilon);

  v1.X() = v1.Y() = v2.X() = v2.Z() = 0.0f;
  v1.Z() = v2.Y() = 1.0f;

  r = v1.Cross(v2);
  EXPECT_EQ(r.X(), -1.0f);
  EXPECT_EQ(r.Y(), 0.0f);
  EXPECT_EQ(r.Z(), 0.0f);

  r = v2.Cross(v1);
  EXPECT_EQ(r.X(), 1.0f);
  EXPECT_EQ(r.Y(), 0.0f);
  EXPECT_EQ(r.Z(), 0.0f);
}

////////////////////////////////////////////////////////////////////////////////
//
// Vec4
//
////////////////////////////////////////////////////////////////////////////////

#include "FasTC/Vector4.h"

TEST(Vector4, BaseFunctionality) {
  FasTC::Vec4f vf;
  FasTC::Vec4d vd;
  vd.X() = 3.0;
  vd.Y() = -10.0;
  vd.Z() = 0.0;
  vd.W() = 100000000.0;

  vf = vd;
  for(int i = 0; i < 4; i++) {
    EXPECT_EQ(vf[i], vd[i]);
  }
}

TEST(Vector4, Accessors) {
  float fv[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  FasTC::Vec4f v4f (fv);
  EXPECT_EQ(v4f.X(), 1.0f);
  EXPECT_EQ(v4f.Y(), 2.0f);
  EXPECT_EQ(v4f.Z(), 3.0f);
  EXPECT_EQ(v4f.W(), 4.0f);

  v4f.X() = 5.0f;
  v4f.Y() = 6.0f;
  v4f.Z() = 7.0f;
  v4f.W() = 8.0f;

  EXPECT_EQ(v4f.X(), 5.0f);
  EXPECT_EQ(v4f.Y(), 6.0f);
  EXPECT_EQ(v4f.Z(), 7.0f);
  EXPECT_EQ(v4f.W(), 8.0f);
}

TEST(Vector4, Addition) {
  float fv[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
  FasTC::Vec4f v4f (fv);

  double dv[4] = { 4.3, -10.2, 0.0f, -22.0f };
  FasTC::Vec4d v3d (dv);

  EXPECT_NEAR((v4f + v3d).X(), 5.3, kEpsilon);
  EXPECT_NEAR((v4f + v3d).Y(), -8.2, kEpsilon);
  EXPECT_NEAR((v4f + v3d).Z(), 3.0, kEpsilon);
  EXPECT_NEAR((v4f + v3d).W(), -18.0, kEpsilon);
}

TEST(Vector4, Swizzle) {
  float fv[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  FasTC::Vec4f v;
  v = fv;

  EXPECT_EQ(v.XXXX().Y(), 1.0f);
  EXPECT_EQ(v.YZXW().X(), 2.0f);
  EXPECT_EQ(v.ZWY().Z(), 2.0f);
  EXPECT_EQ(v.ZZ().X(), 3.0f);
  EXPECT_EQ(v.WWXY().W(), 2.0f);
}
