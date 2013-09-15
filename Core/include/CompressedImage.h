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

#ifndef _COMPRESSED_IMAGE_H_
#define _COMPRESSED_IMAGE_H_

enum ECompressionFormat {
  eCompressionFormat_DXT1,
  eCompressionFormat_DXT5,
  eCompressionFormat_BPTC,

  kNumCompressionFormats
};

#include "Image.h"

class CompressedImage : public Image {

 private:
  ECompressionFormat m_Format;
  uint32 *m_RGBAData;
  uint32 m_DataSz;

  void InitData(const unsigned char *withData);
 public:
  CompressedImage();

  // Create a compressed image from the given data according to
  // the passed format. The size of the data is expected to conform
  // to the width, height, and format specified.
  CompressedImage(
    const unsigned int width, 
    const unsigned int height, 
    const ECompressionFormat format, 
    const unsigned char *data
  );

  CompressedImage( const CompressedImage &other );
  virtual ~CompressedImage();

  virtual void ComputeRGBA();
  virtual const uint32 *GetRGBA() const { return m_RGBAData; }

  // Decompress the compressed image data into outBuf. outBufSz is expected
  // to be the proper size determined by the width, height, and format.
  // !FIXME! We should have a function to explicitly return the in/out buf
  // size for a given compressed image.
  bool DecompressImage(unsigned char *outBuf, unsigned int outBufSz) const;
};

#endif // _COMPRESSED_IMAGE_H_
