/* FasTC
 * Copyright (c) 2013 University of North Carolina at Chapel Hill.
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
#include "Color.h"

static const float kEpsilon = 1e-6;

TEST(Color, DefaultConstructor) {
  FasTC::Color c;
  EXPECT_EQ(c.R(), 0.0f);
  EXPECT_EQ(c.G(), 0.0f);
  EXPECT_EQ(c.B(), 0.0f);
  EXPECT_EQ(c.A(), 0.0f);
}

TEST(Color, AssignmentConstructor) {
  FasTC::Color c(1.0f, 0.0f, 3.0f, -1.0f);
  EXPECT_EQ(c.R(), 1.0f);
  EXPECT_EQ(c.G(), 0.0f);
  EXPECT_EQ(c.B(), 3.0f);
  EXPECT_EQ(c.A(), -1.0f);
}

TEST(Color, VectorOperators) {
  FasTC::Color a(0.1, 0.2, 0.3, 0.4);
  FasTC::Color b(0.2, 0.3, 0.4, 0.5);
  FasTC::Color c = a + b;

  EXPECT_NEAR(c.R(), 0.3, kEpsilon);
  EXPECT_NEAR(c.G(), 0.5, kEpsilon);
  EXPECT_NEAR(c.B(), 0.7, kEpsilon);
  EXPECT_NEAR(c.A(), 0.9, kEpsilon);

  FasTC::Color d = a - b;

  EXPECT_NEAR(d.R(), -0.1, kEpsilon);
  EXPECT_NEAR(d.G(), -0.1, kEpsilon);
  EXPECT_NEAR(d.B(), -0.1, kEpsilon);
  EXPECT_NEAR(d.A(), -0.1, kEpsilon);
}

TEST(Color, EqualityComparison) {
  FasTC::Color a(0.1, 0.2, 0.3, 0.4);
  FasTC::Color b(0.2, 0.3, 0.4, 0.5);

  EXPECT_TRUE(a == a && b == b);
  EXPECT_FALSE(a == b && b == a);
}
