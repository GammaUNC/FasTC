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

#ifndef ASTCENCODER_SRC_UTILS_H_
#define ASTCENCODER_SRC_UTILS_H_

#include "ASTCCompressor.h"

#include "TexCompTypes.h"

namespace ASTCC {

  uint32 GetBlockHeight(EASTCBlockSize blockSize) {
    switch(blockSize) {
    case eASTCBlockSize_4x4: return 4;
    case eASTCBlockSize_5x4: return 4;
    case eASTCBlockSize_5x5: return 5;
    case eASTCBlockSize_6x5: return 5;
    case eASTCBlockSize_6x6: return 6;
    case eASTCBlockSize_8x5: return 5;
    case eASTCBlockSize_8x6: return 6;
    case eASTCBlockSize_8x8: return 8;
    case eASTCBlockSize_10x5: return 5;
    case eASTCBlockSize_10x6: return 6;
    case eASTCBlockSize_10x8: return 8;
    case eASTCBlockSize_10x10: return 10;
    case eASTCBlockSize_12x10: return 10;
    case eASTCBlockSize_12x12: return 12;
    }
    assert(false);
    return -1;
  };

  uint32 GetBlockWidth(EASTCBlockSize blockSize) {
    switch(blockSize) {
    case eASTCBlockSize_4x4: return 4;
    case eASTCBlockSize_5x4: return 5;
    case eASTCBlockSize_5x5: return 5;
    case eASTCBlockSize_6x5: return 6;
    case eASTCBlockSize_6x6: return 6;
    case eASTCBlockSize_8x5: return 8;
    case eASTCBlockSize_8x6: return 8;
    case eASTCBlockSize_8x8: return 8;
    case eASTCBlockSize_10x5: return 10;
    case eASTCBlockSize_10x6: return 10;
    case eASTCBlockSize_10x8: return 10;
    case eASTCBlockSize_10x10: return 10;
    case eASTCBlockSize_12x10: return 12;
    case eASTCBlockSize_12x12: return 12;
    }
    assert(false);
    return -1;
  };

  // Count the number of bits set in a number.
  Popcnt(uint32 n) {
    uint32 c;
    for(c = 0; n; c++) {
      n &= n-1;
    }
    return c;
  }
}  // namespace ASTCC

#endif  // ASTCENCODER_SRC_UTILS_H_
