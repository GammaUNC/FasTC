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

#ifndef PVRTCENCODER_SRC_IMAGE_H_
#define PVRTCENCODER_SRC_IMAGE_H_

#include "FasTC/TexCompTypes.h"
#include "FasTC/Image.h"

#include "FasTC/PVRTCCompressor.h"

#include <vector>

// Forward include
namespace FasTC {
  class Pixel;
}

namespace PVRTCC {

class Image : public FasTC::Image<FasTC::Pixel> {
 public:
  Image(uint32 width, uint32 height);
  Image(uint32 width, uint32 height, const FasTC::Pixel *pixels);
  Image(const Image &);
  Image &operator=(const Image &);
  virtual ~Image();
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
  void AverageDownscale(uint32 xtimes, uint32 ytimes);

  void ComputeHessianEigenvalues(::std::vector<float> &eigOne, 
                                 ::std::vector<float> &eigTwo,
                                 EWrapMode wrapMode = eWrapMode_Wrap);

  void ChangeBitDepth(const uint8 (&depths)[4]);

  void ExpandTo8888();
  void DebugOutput(const char *filename) const;

 private:
  FasTC::Pixel *m_FractionalPixels;

  uint32 GetPixelIndex(int32 i, int32 j, EWrapMode wrapMode = eWrapMode_Clamp) const;
  const FasTC::Pixel &GetPixel(int32 i, int32 j, EWrapMode wrapMode = eWrapMode_Clamp) const;
};

}  // namespace PVRTCC

#endif  // PVRTCENCODER_SRC_IMAGE_H_
