/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill.
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

#include "FasTC/Shapes.h"

#include <cassert>

static const int kAnchorIdx2[BPTCC::kNumShapes2] = {
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15,  2,  8,  2,  2,  8,  8, 15,
  2 ,  8,  2,  2,  8,  8,  2,  2,
  15, 15,  6,  8,  2,  8, 15, 15,
  2 ,  8,  2,  2,  2, 15, 15,  6,
  6 ,  2,  6,  8, 15, 15,  2,  2,
  15, 15, 15, 15, 15,  2,  2, 15
};

static const int kAnchorIdx3[2][BPTCC::kNumShapes3] = {
  {3,  3, 15, 15,  8,  3, 15, 15,
  8 ,  8,  6,  6,  6,  5,  3,  3,
  3 ,  3,  8, 15,  3,  3,  6, 10,
  5 ,  8,  8,  6,  8,  5, 15, 15,
  8 , 15,  3,  5,  6, 10,  8, 15,
  15,  3, 15,  5, 15, 15, 15, 15,
  3 , 15,  5,  5,  5,  8,  5, 10,
  5 , 10,  8, 13, 15, 12,  3,  3 },

  {15,  8,  8,  3, 15, 15,  3,  8,
  15 , 15, 15, 15, 15, 15, 15,  8,
  15 ,  8, 15,  3, 15,  8, 15,  8,
  3  , 15,  6, 10, 15, 15, 10,  8,
  15 ,  3, 15, 10, 10,  8,  9, 10,
  6  , 15,  8, 15,  3,  6,  6,  8,
  15 ,  3, 15, 15, 15, 15, 15, 15,
  15 , 15, 15, 15,  3, 15, 15,  8 }
};

namespace BPTCC {

static uint32 GetAnchorIndexForSubset(
  int subset, const int shapeIdx, const int nSubsets
) {

  int anchorIdx = 0;
  switch(subset) {
    case 1:
    {
      if(nSubsets == 2) {
        anchorIdx = kAnchorIdx2[shapeIdx];
      } else {
        anchorIdx = kAnchorIdx3[0][shapeIdx];
      }
    }
    break;

    case 2:
    {
      assert(nSubsets == 3);
      anchorIdx = kAnchorIdx3[1][shapeIdx];
    }
    break;

    default:
    break;
  }

  return anchorIdx;
}

}  // namespace BPTCC
