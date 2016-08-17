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

#ifndef __FILE_STREAM_H__
#define __FILE_STREAM_H__

#include "FasTC/TexCompTypes.h"

enum EFileMode {
  eFileMode_Read,
  eFileMode_ReadBinary,
  eFileMode_Write,
  eFileMode_WriteBinary,
  eFileMode_WriteAppend,
  eFileMode_WriteBinaryAppend,

  kNumFileModes
};

class FileStreamImpl;
class FileStream {
  
 public:
  FileStream(const CHAR *filename, EFileMode mode);
  FileStream(const FileStream &);
  FileStream &operator=(const FileStream &);
  ~FileStream();

  // Read the contents of the file into the specified buffer. This
  // function returns the number of bytes read on success. It returns
  // -2 if the file was opened with an incompatible mode and -1 on
  // platform specific error.
  int32 Read(uint8 *buf, uint32 bufSz);

  // Write the contents of buf to the filestream. This function returns
  // the number of bytes written on success. It returns -2 if the file
  // was opened with an incompatible mode and -1 on platform specific
  // error.
  int32 Write(const uint8 *buf, uint32 bufSz);

  // Returns where in the filestream we are. Returns -1 on error.
  int32 Tell();
  
  enum ESeekPosition {
    eSeekPosition_Beginning,
    eSeekPosition_Current,
    eSeekPosition_End
  };

  // Repositions the stream to the specified offset away from the 
  // position in the stream. This function will always fail if the 
  // file mode is append. Otherwise, it returns true on success.
  bool Seek(uint32 offset, ESeekPosition pos);

  // Flush the data of the stream. This function is platform specific.
  void Flush();

  const CHAR *GetFilename() const { return m_Filename; }

 private:

  // Platform specific implementation 
  FileStreamImpl *m_Impl;

  EFileMode m_Mode;

  static const uint32 kMaxFilenameSz = 256;
  CHAR m_Filename[kMaxFilenameSz];
};

#endif // __FILE_STREAM_H__
