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

#ifndef BPTCENCODER_INCLUDE_SHAPES_H_
#define BPTCENCODER_INCLUDE_SHAPES_H_

namespace BPTCC {

  static const uint32 kNumShapes2 = 64;
  static const uint16 kShapeMask2[kNumShapes2] = {
    0xcccc, 0x8888, 0xeeee, 0xecc8, 0xc880, 0xfeec, 0xfec8, 0xec80,
    0xc800, 0xffec, 0xfe80, 0xe800, 0xffe8, 0xff00, 0xfff0, 0xf000,
    0xf710, 0x008e, 0x7100, 0x08ce, 0x008c, 0x7310, 0x3100, 0x8cce,
    0x088c, 0x3110, 0x6666, 0x366c, 0x17e8, 0x0ff0, 0x718e, 0x399c,
    0xaaaa, 0xf0f0, 0x5a5a, 0x33cc, 0x3c3c, 0x55aa, 0x9696, 0xa55a,
    0x73ce, 0x13c8, 0x324c, 0x3bdc, 0x6996, 0xc33c, 0x9966, 0x0660,
    0x0272, 0x04e4, 0x4e40, 0x2720, 0xc936, 0x936c, 0x39c6, 0x639c,
    0x9336, 0x9cc6, 0x817e, 0xe718, 0xccf0, 0x0fcc, 0x7744, 0xee22
  };

  static const uint32 kNumShapes3 = 64;
  static const uint16 kShapeMask3[kNumShapes3][2] = {
    {0xfecc, 0xf600}, {0xffc8, 0x7300}, {0xff90, 0x3310}, {0xecce, 0x00ce},
    {0xff00, 0xcc00}, {0xcccc, 0xcc00}, {0xffcc, 0x00cc}, {0xffcc, 0x3300},
    {0xff00, 0xf000}, {0xfff0, 0xf000}, {0xfff0, 0xff00}, {0xcccc, 0x8888},
    {0xeeee, 0x8888}, {0xeeee, 0xcccc}, {0xffec, 0xec80}, {0x739c, 0x7310},
    {0xfec8, 0xc800}, {0x39ce, 0x3100}, {0xfff0, 0xccc0}, {0xfccc, 0x0ccc},
    {0xeeee, 0xee00}, {0xff88, 0x7700}, {0xeec0, 0xcc00}, {0x7730, 0x3300},
    {0x0cee, 0x00cc}, {0xffcc, 0xfc88}, {0x6ff6, 0x0660}, {0xff60, 0x6600},
    {0xcbbc, 0xc88c}, {0xf966, 0xf900}, {0xceec, 0x0cc0}, {0xff10, 0x7310},
    {0xff80, 0xec80}, {0xccce, 0x08ce}, {0xeccc, 0xec80}, {0x6666, 0x4444},
    {0x0ff0, 0x0f00}, {0x6db6, 0x4924}, {0x6bd6, 0x4294}, {0xcf3c, 0x0c30},
    {0xc3fc, 0x03c0}, {0xffaa, 0xff00}, {0xff00, 0x5500}, {0xfcfc, 0xcccc},
    {0xcccc, 0x0c0c}, {0xf6f6, 0x6666}, {0xaffa, 0x0ff0}, {0xfff0, 0x5550},
    {0xfaaa, 0xf000}, {0xeeee, 0x0e0e}, {0xf8f8, 0x8888}, {0xfff0, 0x9990},
    {0xeeee, 0xe00e}, {0x8ff8, 0x8888}, {0xf666, 0xf000}, {0xff00, 0x9900},
    {0xff66, 0xff00}, {0xcccc, 0xc00c}, {0xcffc, 0xcccc}, {0xf000, 0x9000},
    {0x8888, 0x0808}, {0xfefe, 0xeeee}, {0xfffa, 0xfff0}, {0x7bde, 0x7310}
  };

  static uint8 GetSubsetForIndex(int idx, const int shapeIdx, const int nSubs) {
    int subset = 0;

    switch(nSubs) {
    case 2:
      {
        subset = !!((1 << idx) & kShapeMask2[shapeIdx]);
      }
      break;

    case 3:
      {
        if(1 << idx & kShapeMask3[shapeIdx][0])
          subset = 1 + !!((1 << idx) & kShapeMask3[shapeIdx][1]);
        else
          subset = 0;
      }
      break;

    default:
      break;
    }

    return subset;
  }

}  // namespace BPTCC

#endif  // BPTCENCODER_INCLUDE_SHAPES_H_
