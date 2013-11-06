/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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

#ifndef FASTC_BASE_INCLUDE_IMAGE_H_
#define FASTC_BASE_INCLUDE_IMAGE_H_

#include "TexCompTypes.h"
#include "ImageFwd.h"

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
      for(uint32 j = 0; j < other.GetWidth(); j++) {
        for(uint32 i = 0; i < other.GetHeight(); i++) {
          other(i, j).Unpack((*this)(i, j).Pack());
        }
      }
    }

    double ComputePSNR(Image<PixelType> *other);
    double ComputeSSIM(Image<PixelType> *other);

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

}  // namespace FasTC

#endif // __TEXCOMP_IMAGE_H__
