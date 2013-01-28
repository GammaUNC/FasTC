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

#include "CompressedImage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "TexCompTypes.h"
#include "BC7Compressor.h"

CompressedImage::CompressedImage()
  : m_Width(0)
  , m_Height(0)
  , m_Format(ECompressionFormat(-1))
  , m_Data(0)
  , m_DataSz(0)
{ }

CompressedImage::CompressedImage( const CompressedImage &other )
  : m_Width(other.m_Width)
  , m_Height(other.m_Height)
  , m_Format(other.m_Format)
  , m_Data(0)
  , m_DataSz(0)
{
  InitData(other.m_Data);
}

CompressedImage::CompressedImage(
  const unsigned int width,				 
  const unsigned int height,
  const ECompressionFormat format,
  const unsigned char *data
) 
: m_Width(width)
, m_Height(height)
, m_Format(format)
, m_Data(0)
, m_DataSz(0)
{
  InitData(data);
}

void CompressedImage::InitData(const unsigned char *withData) {
  m_DataSz = 0;
  int uncompDataSz = m_Width * m_Height * 4;

  switch(m_Format) {
    case eCompressionFormat_DXT1: m_DataSz = uncompDataSz / 8; break;
    case eCompressionFormat_DXT5: m_DataSz = uncompDataSz / 4; break;
    case eCompressionFormat_BPTC: m_DataSz = uncompDataSz / 4; break;
  }
  
  if(m_DataSz > 0) {
    m_Data = new unsigned char[m_DataSz];
    memcpy(m_Data, withData, m_DataSz);
  }
}

CompressedImage::~CompressedImage() {
  if(m_Data) {
    delete [] m_Data;
    m_Data = NULL;
  }
}

bool CompressedImage::DecompressImage(unsigned char *outBuf, unsigned int outBufSz) const {

  // First make sure that we have enough data
  uint32 dataSz = 0;
  switch(m_Format) {
    case eCompressionFormat_DXT1: dataSz = m_DataSz * 8; break;
    case eCompressionFormat_DXT5: dataSz = m_DataSz * 4; break;
    case eCompressionFormat_BPTC: dataSz = m_DataSz * 4; break;
  }

  if(dataSz > outBufSz) {
    fprintf(stderr, "Not enough space to store entire decompressed image! "
                    "Got %d bytes, but need %d!\n", outBufSz, dataSz);
    return false;
  }

  switch(m_Format) {
  case eCompressionFormat_BPTC: 
    BC7C::DecompressImageBC7(m_Data, outBuf, m_Width, m_Height);
    break;

  default:
    const char *errStr = "Have not implemented decompression method.";
    fprintf(stderr, "%s\n", errStr);
    assert(!errStr);
    return false;
  }

  return true;
}
