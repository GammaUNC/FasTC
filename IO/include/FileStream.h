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

#ifndef __FILE_STREAM_H__
#define __FILE_STREAM_H__

#include "TexCompTypes.h"

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
