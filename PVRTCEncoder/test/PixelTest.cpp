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
#include "Pixel.h"

TEST(Pixel, DefaultConstructor) {
  PVRTCC::Pixel p;
  EXPECT_EQ(p.R(), 0);
  EXPECT_EQ(p.G(), 0);
  EXPECT_EQ(p.B(), 0);
  EXPECT_EQ(p.A(), 0);

  uint8 depth[4];
  p.GetBitDepth(depth);

  for(int i = 0; i < 4; i++) {
    EXPECT_EQ(depth[i], 8);
  }
}

TEST(Pixel, FromBitsAndAssociatedConstructor) {
  const uint8 bits[8] = { 0xA8, 0xB3, 0x7C, 0x21, 0xBD, 0xD4, 0x09, 0x92 };
  PVRTCC::Pixel ps[2];
  ps[0] = PVRTCC::Pixel(bits);
  ps[1].FromBits(bits);

  for(int i = 0; i < 2; i++) {
    PVRTCC::Pixel &p = ps[i];

    EXPECT_EQ(p.A(), 0xA8);
    EXPECT_EQ(p.R(), 0xB3);
    EXPECT_EQ(p.G(), 0x7C);
    EXPECT_EQ(p.B(), 0x21);

    uint8 depth[4];
    p.GetBitDepth(depth);
    for(int j = 0; j < 4; j++) {
      EXPECT_EQ(depth[j], 8);
    }
  }
}
