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

#ifndef FASTC_BASE_INCLUDE_IMAGE_H_
#define FASTC_BASE_INCLUDE_IMAGE_H_

#include "FasTC/TexCompTypes.h"
#include "FasTC/ImageFwd.h"

namespace FasTC {

  class IPixel;

  template<typename U, typename V>
  extern double ComputePSNR(Image<U> *img1, Image<V> *img2);

  // Forward declare
  template<typename PixelType>
  class Image {

   public:
    Image() : m_Width(0), m_Height(0), m_Pixels(0) { }
    Image(uint32 width, uint32 height);
    Image(uint32 width, uint32 height,
          const PixelType *pixels);
    Image(uint32 width, uint32 height,
          const uint32 *pixels);
    Image(const Image<PixelType> &);
    Image &operator=(const Image<PixelType> &);
    virtual ~Image();

    virtual Image *Clone() const {
      return new Image(*this);
    };

    PixelType &operator()(uint32 i, uint32 j);
    const PixelType &operator()(uint32 i, uint32 j) const;

    // Reads a buffer full of pixels and stores them in the
    // data associated with this image.
    virtual bool ReadPixels(const uint32 *rgba);
    const PixelType *GetPixels() const { return m_Pixels; }

    uint32 GetWidth() const { return m_Width; }
    uint32 GetHeight() const { return m_Height; }
    uint32 GetNumPixels() const { return GetWidth() * GetHeight(); }

    template<typename OtherPixelType>
    void ConvertTo(Image<OtherPixelType> &other) const {
      for(uint32 j = 0; j < other.GetHeight(); j++) {
        for(uint32 i = 0; i < other.GetWidth(); i++) {
          other(i, j).Unpack((*this)(i, j).Pack());
        }
      }
    }

    double ComputePSNR(Image<PixelType> *other);
    double ComputeSSIM(Image<PixelType> *other);

    Image<PixelType> Diff(Image<PixelType> *other, float mult);

    double ComputeEntropy();
    double ComputeMeanLocalEntropy();
    
    // Function to allow derived classes to populate the pixel array.
    // This may involve decompressing a compressed image or otherwise
    // processing some data in order to populate the m_Pixels pointer.
    // This function should use SetImageData in order to set all of the
    // appropriate pixels.
    virtual void ComputePixels() { }

    // Filters the image with a given set of kernel values. The values
    // are normalized before they are used (i.e. we make sure that they
    // sum up to one).
    void Filter(const Image<IPixel> &kernel);

   private:
    uint32 m_Width;
    uint32 m_Height;

    PixelType *m_Pixels;

   protected:

    void SetImageData(uint32 width, uint32 height, PixelType *data);
  };

  extern void GenerateGaussianKernel(Image<IPixel> &out, uint32 size, float sigma);

  template <typename PixelType>
  extern void SplitChannels(const Image<PixelType> &in,
                            Image<IPixel> *channelOne,
                            Image<IPixel> *channelTwo,
                            Image<IPixel> *channelThree);

  extern void DiscreteCosineXForm(Image<IPixel> *img, uint32 blockSize);
  extern void InvDiscreteCosineXForm(Image<IPixel> *img, uint32 blockSize);
}  // namespace FasTC

#endif // __TEXCOMP_IMAGE_H__
