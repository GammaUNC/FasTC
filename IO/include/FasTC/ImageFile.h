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

#ifndef _IMAGE_FILE_H_ 
#define _IMAGE_FILE_H_ 

#include "FasTC/TexCompTypes.h"
#include "FasTC/ImageFwd.h"

#include "ImageFileFormat.h"

// Forward declare
class CompressedImage;
struct SCompressionSettings;

// Class definition
class ImageFile {

public:

  // Opens and loads an image file from the given path. The file format
  // is inferred from the filename.
  ImageFile(const char *filename);

  // Opens and loads a given image file with the passed format.
  ImageFile(const char *filename, EImageFileFormat format);

  // Creates an imagefile with the corresponding image data. This is ready
  // to be written to disk with the passed filename.
  ImageFile(const char *filename, EImageFileFormat format, const FasTC::Image<> &);

  ~ImageFile();

  static EImageFileFormat DetectFileFormat(const CHAR *filename);
  unsigned int GetWidth() const { return m_Width; }
  unsigned int GetHeight() const { return m_Height; }
  FasTC::Image<> *GetImage() const { return m_Image; }

  // Loads the image into memory. If this function returns true, then a valid
  // m_Image will be created and available.
  bool Load();

  // Writes the given image to disk. Returns true on success.
  bool Write();

 private:

  static const unsigned int kMaxFilenameSz = 256;
  char m_Filename[kMaxFilenameSz];
  unsigned int m_Width;
  unsigned int m_Height;

  const EImageFileFormat m_FileFormat;

  uint8 *m_FileData;
  int32 m_FileDataSz;

  FasTC::Image<> *m_Image;
  
  bool ReadFileData(const CHAR *filename);
  static bool WriteImageDataToFile(const uint8 *data, const uint32 dataSz, const CHAR *filename);

  FasTC::Image<> *LoadImage() const;
};
#endif // _IMAGE_FILE_H_ 
