#include "CompressedImage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
  int dataSz = 0;
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
