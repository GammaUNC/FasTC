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

#ifndef PVRTCENCODER_SRC_INDEXER_H_
#define PVRTCENCODER_SRC_INDEXER_H_

#include "FasTC/PVRTCCompressor.h"

#include <algorithm>
#include <cassert>

namespace PVRTCC {

class Indexer {
 private:
  const EWrapMode m_WrapMode;
  const uint32 m_Width;
  const uint32 m_Height;

  uint32 Resolve(int32 i, uint32 limit) const {
    int32 l = static_cast<int32>(limit);

    // Assumptions
    assert(i > -l);
    assert(i < 2*l);

    int32 r;
    switch(m_WrapMode) {
    case eWrapMode_Clamp:
      r = static_cast<uint32>(std::max(0, std::min(i, l-1)));
      break;

    case eWrapMode_Wrap:
      {
	r = i;
	if ((l & (l-1)) == 0) {
	  r = (r + l) & (l - 1);
	} else {
	  if (r >= l) { r -= l; }
	  if (r <  0) { r += l; }
	}
      }
      break;
    }

    assert (r >= 0);
    assert (r < limit);
    return r;
  }

 public:
  Indexer(uint32 width, uint32 height, EWrapMode wrapMode = eWrapMode_Wrap)
    : m_WrapMode(wrapMode)
    , m_Width(width)
    , m_Height(height)
  { }

  uint32 GetWidth() const { return this->m_Width; }
  uint32 GetHeight() const { return this->m_Height; }
  EWrapMode GetWrapMode() const { return this->m_WrapMode; }

  uint32 ResolveX(int32 i) const { return Resolve(i, this->m_Width); }
  uint32 ResolveY(int32 i) const { return Resolve(i, this->m_Height); }

  uint32 operator()(int32 i, int32 j) const {
    uint32 _i = this->ResolveX(i);
    uint32 _j = this->ResolveY(j);

    uint32 index = _j * this->m_Width + _i;
    assert (index < this->m_Width * this->m_Height);
    assert (index >= 0);
    return index;
  }
};

}  // namespace PVRTCC

#endif  // PVRTCENCODER_SRC_INDEXER_H_
