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

#ifndef PVRTCENCODER_SRC_IMAGE_H_
#define PVRTCENCODER_SRC_IMAGE_H_

#include "TexCompTypes.h"
#include "PVRTCCompressor.h"

#include <vector>

namespace PVRTCC {

class Pixel;

class Image {
 public:
  Image(uint32 height, uint32 width);
  Image(uint32 height, uint32 width, const Pixel *pixels);
  Image(const Image &);
  Image &operator=(const Image &);
  ~Image();

  void BilinearUpscale(uint32 xtimes, uint32 ytimes,
                       EWrapMode wrapMode = eWrapMode_Wrap);

  // Downscales the image by taking an anisotropic diffusion approach
  // with respect to the gradient of the intensity. In this way, we can
  // preserve the most important image structures by not blurring across
  // edge boundaries, which when upscaled will retain the structural
  // image quality...
  void ContentAwareDownscale(uint32 xtimes, uint32 ytimes,
                             EWrapMode wrapMode = eWrapMode_Wrap,
                             bool bOffsetNewPixels = false);

  // Downscales the image by using a simple averaging of the neighboring pixel values
  void AverageDownscale(uint32 xtimes, uint32 ytimes,
                        EWrapMode wrapMode = eWrapMode_Wrap);

  void ComputeHessianEigenvalues(::std::vector<float> &eigOne, 
                                 ::std::vector<float> &eigTwo,
                                 EWrapMode wrapMode = eWrapMode_Wrap);

  void ChangeBitDepth(const uint8 (&depths)[4]);
  void ExpandTo8888();

  Pixel &operator()(uint32 i, uint32 j);
  const Pixel &operator()(uint32 i, uint32 j) const;

  uint32 GetWidth() const { return m_Width; }
  uint32 GetHeight() const { return m_Height; }

  void DebugOutput(const char *filename) const;

 private:
  uint32 m_Width;
  uint32 m_Height;
  Pixel *m_Pixels;
  Pixel *m_FractionalPixels;

  const uint32 GetPixelIndex(int32 i, int32 j, EWrapMode wrapMode = eWrapMode_Clamp) const;
  const Pixel &GetPixel(int32 i, int32 j, EWrapMode wrapMode = eWrapMode_Clamp) const;
};

}  // namespace PVRTCC

#endif  // PVRTCENCODER_SRC_IMAGE_H_
