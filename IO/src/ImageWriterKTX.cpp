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

#include "ImageWriterKTX.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "Image.h"
#include "Pixel.h"

#include "CompressedImage.h"
#include "GLDefines.h"

ImageWriterKTX::ImageWriterKTX(FasTC::Image<> &im)
  : ImageWriter(im.GetWidth(), im.GetHeight(), NULL)
  , m_Image(im)
{ }

class ByteWriter {
 private:
  uint8 *m_Base;
  uint8 *m_Head;
  uint32 m_BytesWritten;
  uint32 m_BufferSz;
 public:
  ByteWriter(uint8 *dst, uint32 sz)
  : m_Base(dst), m_Head(dst), m_BytesWritten(0), m_BufferSz(dst? sz : 0) { }

  uint8 *GetBytes() const { return m_Base; }
  uint32 GetBytesWritten() const { return m_BytesWritten; }

  void Write(const void *src, const uint32 nBytes) {
    while(m_BytesWritten + nBytes > m_BufferSz) {
      m_BufferSz <<= 1;
      uint8 *newBuffer = new uint8[m_BufferSz];
      memcpy(newBuffer, m_Base, m_BytesWritten);
      delete m_Base;
      m_Base = newBuffer;
      m_Head = m_Base + m_BytesWritten;
    }

    memcpy(m_Head, src, nBytes);
    m_Head += nBytes;
    m_BytesWritten += nBytes;
  }

  void Write(const uint32 v) {
    Write(&v, 4);
  }
};

bool ImageWriterKTX::WriteImage() {
  ByteWriter wtr (m_RawFileData, m_RawFileDataSz);

  const uint8 kIdentifier[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
  };
  wtr.Write(kIdentifier, 12);
  wtr.Write(0x04030201);

  const char *orientationKey = "KTXorientation";
  uint32 oKeyLen = static_cast<uint32>(strlen(orientationKey));

  const char *orientationValue = "S=r,T=d";
  uint32 oValLen = static_cast<uint32>(strlen(orientationValue));

  const uint32 kvSz = oKeyLen + 1 + oValLen + 1;
  uint32 tkvSz = kvSz + 4; // total kv size
  tkvSz = (tkvSz + 3) & ~0x3; // 4-byte aligned

  CompressedImage *ci = dynamic_cast<CompressedImage *>(&m_Image);
  if(ci) {
    wtr.Write(0);  // glType
    wtr.Write(1);  // glTypeSize
    wtr.Write(0);  // glFormat must be zero for compressed images...
    switch(ci->GetFormat()) {
    case FasTC::eCompressionFormat_BPTC:
      wtr.Write(GL_COMPRESSED_RGBA_BPTC_UNORM);  // glInternalFormat
      wtr.Write(GL_RGBA);  // glBaseFormat
      break;

    case FasTC::eCompressionFormat_PVRTC4:
      wtr.Write(COMPRESSED_RGBA_PVRTC_4BPPV1_IMG);  // glInternalFormat
      wtr.Write(GL_RGBA);  // glBaseFormat
      break;

    default:
      fprintf(stderr, "Unsupported KTX compressed format: %d\n", ci->GetFormat());
      return false;
    }
  } else {
    wtr.Write(GL_BYTE);  // glType
    wtr.Write(1);  // glTypeSize
    wtr.Write(GL_RGBA);  // glFormat
    wtr.Write(GL_RGBA8);  // glInternalFormat
    wtr.Write(GL_RGBA);  // glBaseFormat
  }

  wtr.Write(m_Width);  // pixelWidth
  wtr.Write(m_Height); // pixelHeight
  wtr.Write(0);        // pixelDepth
  wtr.Write(0);        // numberOfArrayElements
  wtr.Write(1);        // numberOfFaces
  wtr.Write(1);        // numberOfMipmapLevels
  wtr.Write(tkvSz);    // total key value size
  wtr.Write(kvSz);     // key value size
  wtr.Write(orientationKey, oKeyLen + 1); // key
  wtr.Write(orientationValue, oValLen + 1); // value
  wtr.Write(orientationKey, tkvSz - kvSz - 4); // padding

  if(ci && ci->GetFormat() == FasTC::eCompressionFormat_BPTC) {
    static const uint32 kImageSize = m_Width * m_Height;
    wtr.Write(kImageSize); // imageSize
    wtr.Write(ci->GetCompressedData(), kImageSize); // imagedata...
  } else if(ci && ci->GetFormat() == FasTC::eCompressionFormat_PVRTC4) {
    static const uint32 kImageSize = m_Width * m_Height >> 1;
    wtr.Write(kImageSize); // imageSize
    wtr.Write(ci->GetCompressedData(), kImageSize); // imagedata...
  } else {
    static const uint32 kImageSize = m_Width * m_Height * 4;
    wtr.Write(kImageSize); // imageSize
    wtr.Write(m_Image.GetPixels(), kImageSize); // imagedata...
  }

  m_RawFileData = wtr.GetBytes();
  m_RawFileDataSz = wtr.GetBytesWritten();
  return true;
}
