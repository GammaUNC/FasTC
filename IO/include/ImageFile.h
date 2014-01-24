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

#ifndef _IMAGE_FILE_H_ 
#define _IMAGE_FILE_H_ 

#include "TexCompTypes.h"
#include "ImageFileFormat.h"
#include "ImageFwd.h"

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
  unsigned int m_Handle;
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
