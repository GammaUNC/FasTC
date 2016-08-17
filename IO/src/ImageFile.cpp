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

#include "FasTC/ImageFile.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cassert>
#include <algorithm>

#include "FasTC/ImageWriter.h"
#include "FasTC/ImageLoader.h"
#include "FasTC/CompressedImage.h"
#include "FasTC/Image.h"
#include "FasTC/FileStream.h"

#ifdef PNG_FOUND
#  include "ImageLoaderPNG.h"
#  include "ImageWriterPNG.h"
#endif

#ifdef PVRTEXLIB_FOUND
#  include "ImageLoaderPVR.h"
#endif

#include "ImageLoaderTGA.h"
#include "ImageLoaderASTC.h"

#include "ImageLoaderKTX.h"
#include "ImageWriterKTX.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
// Static helper functions
//
//////////////////////////////////////////////////////////////////////////////////////////

static inline void ReportError(const CHAR *msg) {
  fprintf(stderr, "ImageFile -- %s\n", msg);
}

template <typename T>
static inline T abs(const T &a) {
  return a > 0? a : -a;
}

//!HACK!
#ifdef _MSC_VER
#define strncpy strncpy_s
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
// ImageFile implementation
//
//////////////////////////////////////////////////////////////////////////////////////////

ImageFile::ImageFile(const CHAR *filename)
  : m_FileFormat(  DetectFileFormat(filename) )
  , m_FileData(NULL)
  , m_FileDataSz(-1)
  , m_Image(NULL)
{ 
  strncpy(m_Filename, filename, kMaxFilenameSz);
}

ImageFile::ImageFile(const CHAR *filename, EImageFileFormat format)
  : m_FileFormat(format)
  , m_FileData(NULL)
  , m_FileDataSz(-1)
  , m_Image(NULL)
{ 
  strncpy(m_Filename, filename, kMaxFilenameSz);
}

ImageFile::ImageFile(const char *filename, EImageFileFormat format, const FasTC::Image<> &image)
  : m_FileFormat(format)
  , m_FileData(NULL)
  , m_FileDataSz(-1)
  , m_Image(image.Clone())
{
  strncpy(m_Filename, filename, kMaxFilenameSz);
}

ImageFile::~ImageFile() { 
  if(m_Image) {
    delete m_Image;
    m_Image = NULL;
  }

  if(m_FileData) {
    delete [] m_FileData;
    m_FileData = NULL;
  }
}

bool ImageFile::Load() {

  if(m_Image) {
    delete m_Image;
    m_Image = NULL;
  }
  
  if(m_FileData) {
    delete [] m_FileData;
    m_FileData = NULL;
  }

  if(ReadFileData(m_Filename)) {
    m_Image = LoadImage();
  }

  return m_Image != NULL;
}

bool ImageFile::Write() {

  ImageWriter *writer = NULL;
  switch(m_FileFormat) {

#ifdef PNG_FOUND
    case eFileFormat_PNG:
      writer = new ImageWriterPNG(*m_Image);
      break;
#endif // PNG_FOUND

    case eFileFormat_KTX:
      writer = new ImageWriterKTX(*m_Image);
      break;

  default:
    fprintf(stderr, "Unable to write image: unknown file format.\n");
    return false;
  }

  if(NULL == writer)
    return false;

  if(!writer->WriteImage()) {
    delete writer;
    return false;
  }

  WriteImageDataToFile(writer->GetRawFileData(), uint32(writer->GetRawFileDataSz()), m_Filename);

  delete writer;
  return true;
}

FasTC::Image<> *ImageFile::LoadImage() const {

  ImageLoader *loader = NULL;
  switch(m_FileFormat) {

#ifdef PNG_FOUND
    case eFileFormat_PNG:
      loader = new ImageLoaderPNG(m_FileData);
      break;
#endif // PNG_FOUND

#ifdef PVRTEXLIB_FOUND
    case eFileFormat_PVR:
      loader = new ImageLoaderPVR(m_FileData);
      break;
#endif // PVRTEXLIB_FOUND

    case eFileFormat_TGA:
      loader = new ImageLoaderTGA(m_FileData, m_FileDataSz);
      break;

    case eFileFormat_KTX:
      loader = new ImageLoaderKTX(m_FileData, m_FileDataSz);
      break;

    case eFileFormat_ASTC:
      loader = new ImageLoaderASTC(m_FileData, m_FileDataSz);
      break;

    default:
      fprintf(stderr, "Unable to load image: unknown file format.\n");
      return NULL;
  }

  if(!loader)
    return NULL;

  FasTC::Image<> *i = loader->LoadImage();
  if(i == NULL) {
    fprintf(stderr, "Unable to load image!\n");
  }

  // Cleanup
  delete loader;

  return i;
}

EImageFileFormat ImageFile::DetectFileFormat(const CHAR *filename) {

  size_t len = strlen(filename);
  if(len >= 256) {
    assert(false);
    ReportError("Filename too long!");
    return kNumImageFileFormats;
  }

  size_t dotPos = len - 1;

  while((dotPos >= len)? false : filename[dotPos--] != '.');
  if (dotPos >= len) {
    assert(!"Malformed filename... no .ext");
    return kNumImageFileFormats;
  }
  
  // consume the last character...
  dotPos++;

  const CHAR *ext = &filename[dotPos];

  if(strcmp(ext, ".png") == 0) {
    return eFileFormat_PNG;
  }
  else if(strcmp(ext, ".pvr") == 0) {
    return eFileFormat_PVR;
  }
  else if(strcmp(ext, ".tga") == 0) {
    return eFileFormat_TGA;
  }
  else if(strcmp(ext, ".ktx") == 0) {
    return eFileFormat_KTX;
  }
  else if(strcmp(ext, ".astc") == 0) {
    return eFileFormat_ASTC;
  }

  return kNumImageFileFormats;
}

bool ImageFile::ReadFileData(const CHAR *filename) {
  FileStream fstr (filename, eFileMode_ReadBinary);
  if(fstr.Tell() < 0) {
    fprintf(stderr, "Error opening file for reading: %s\n", filename);
    return 0;
  }

  // Figure out the filesize.
  fstr.Seek(0, FileStream::eSeekPosition_End);
  uint32 fileSize = fstr.Tell();

  // Allocate data for file contents
  m_FileData = new unsigned char[fileSize];

  // Return stream to beginning of file
  fstr.Seek(0, FileStream::eSeekPosition_Beginning);
  assert(fstr.Tell() == 0);

  // Read all of the data
  uint64 totalBytesRead = 0;
  uint64 totalBytesLeft = fileSize;
  int32 bytesRead;
  while((bytesRead = fstr.Read(m_FileData, uint32(fileSize))) > 0) {
    totalBytesRead += bytesRead;
    totalBytesLeft -= bytesRead;
  }

  if(totalBytesRead != fileSize) {
    assert(!"We didn't read as much data as we thought we had!");
    fprintf(stderr, "Internal error: Incorrect file size assumption\n");
    return false;
  }

  m_FileDataSz = fileSize;
  return true;
}

bool ImageFile::WriteImageDataToFile(const uint8 *data,
                                     const uint32 dataSz,
                                     const CHAR *filename) {

  // Open a file stream and write out the data...
  FileStream fstr (filename, eFileMode_WriteBinary);
  if(fstr.Tell() < 0) {
    fprintf(stderr, "Error opening file for reading: %s\n", filename);
    return 0;
  }

  fstr.Write(data, dataSz);
  fstr.Flush();
  return true;
}
