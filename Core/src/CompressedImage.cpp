#include "CompressedImage.h"

#include <string.h>
#include <stdlib.h>

CompressedImage::CompressedImage( const CompressedImage &other )
  : m_Width(other.m_Width)
  , m_Height(other.m_Height)
  , m_Format(other.m_Format)
  , m_Data(0)
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
{
  InitData(data);
}

void CompressedImage::InitData(const unsigned char *withData) {
  unsigned int dataSz = 0;
  int uncompDataSz = m_Width * m_Height * 4;

  switch(m_Format) {
    case eCompressionFormat_DXT1: dataSz = uncompDataSz / 8; break;
    case eCompressionFormat_DXT5: dataSz = uncompDataSz / 4; break;
    case eCompressionFormat_BPTC: dataSz = uncompDataSz / 4; break;
  }
  
  if(dataSz > 0) {
    m_Data = new unsigned char[dataSz];
    memcpy(m_Data, withData, dataSz);
  }
}

CompressedImage::~CompressedImage() {
  if(m_Data) {
    delete [] m_Data;
    m_Data = NULL;
  }
}
