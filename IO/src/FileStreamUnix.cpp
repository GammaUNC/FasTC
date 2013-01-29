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

#include "FileStream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

class FileStreamImpl {
  
private:
  // Track the number of references to this filestream so
  // that we know whether or not we need to close it when
  // the object gets destroyed.
  uint32 m_ReferenceCount;

  FILE *m_FilePtr;

public:
  FileStreamImpl(const CHAR *filename, EFileMode mode) 
    : m_ReferenceCount(1)
  {

    const char *modeStr = "r";
    switch(mode) {
    default:
    case eFileMode_Read:
      // Do nothing.
      break;

    case eFileMode_ReadBinary:
      modeStr = "rb";
      break;

    case eFileMode_Write:
      modeStr = "w";
      break;

    case eFileMode_WriteBinary:
      modeStr = "wb";
      break;

    case eFileMode_WriteAppend:
      modeStr = "a";
      break;

    case eFileMode_WriteBinaryAppend:
      modeStr = "ab";
      break;
    }

    m_FilePtr = fopen(filename, modeStr);
  }

  ~FileStreamImpl() {
    fclose(m_FilePtr);
  }

  void IncreaseReferenceCount() { m_ReferenceCount++; }
  void DecreaseReferenceCount() { m_ReferenceCount--; }

  uint32 GetReferenceCount() { return m_ReferenceCount; }

  FILE *GetFilePtr() const { return m_FilePtr; }
};

FileStream::FileStream(const CHAR *filename, EFileMode mode)
  : m_Impl(new FileStreamImpl(filename, mode))
  , m_Mode(mode)
{
  strncpy(m_Filename, filename, kMaxFilenameSz);
  m_Filename[kMaxFilenameSz - 1] = '\0';
}
    
FileStream::FileStream(const FileStream &other)
  : m_Impl(other.m_Impl)
  , m_Mode(other.m_Mode)
{
  m_Impl->IncreaseReferenceCount();
  strncpy(m_Filename, other.m_Filename, kMaxFilenameSz);
}

FileStream &FileStream::operator=(const FileStream &other) {
  
  // We no longer reference this implementation.
  m_Impl->DecreaseReferenceCount();

  // If we're the last ones to reference it, then it should be destroyed.
  if(m_Impl->GetReferenceCount() <= 0) {
    assert(m_Impl->GetReferenceCount() == 0);
    delete m_Impl;
    m_Impl = 0;
  }

  m_Impl = other.m_Impl;
  m_Impl->IncreaseReferenceCount();

  m_Mode = other.m_Mode;
  strncpy(m_Filename, other.m_Filename, kMaxFilenameSz);

  return *this;
}

FileStream::~FileStream() {
  // We no longer reference this implementation.
  m_Impl->DecreaseReferenceCount();

  // If we're the last ones to reference it, then it should be destroyed.
  if(m_Impl->GetReferenceCount() <= 0) {
    assert(m_Impl->GetReferenceCount() == 0);
    delete m_Impl;
    m_Impl = 0;
  }  
}

int32 FileStream::Read(uint8 *buf, uint32 bufSz) {

  if(
     m_Mode == eFileMode_Write ||
     m_Mode == eFileMode_WriteBinary ||
     m_Mode == eFileMode_WriteAppend ||
     m_Mode == eFileMode_WriteBinaryAppend
  ) {
    fprintf(stderr, "Cannot read from file '%s': File opened for reading.", m_Filename);
    return -2;
  }

  FILE *fp = m_Impl->GetFilePtr();
  if(NULL == fp)
    return -1;

  uint32 amtRead = fread(buf, 1, bufSz, fp);
  if(amtRead != bufSz && !feof(fp)) {
    fprintf(stderr, "Error reading from file '%s'.", m_Filename);
    return -1;
  }

  return amtRead;
}

int32 FileStream::Write(const uint8 *buf, uint32 bufSz) {
  if(
     m_Mode == eFileMode_Read ||
     m_Mode == eFileMode_ReadBinary
  ) {
    fprintf(stderr, "Cannot write to file '%s': File opened for writing.", m_Filename);
    return -2;
  }

  FILE *fp = m_Impl->GetFilePtr();
  if(NULL == fp)
    return -1;

  uint32 amtWritten = fwrite(buf, 1, bufSz, fp);
  if(amtWritten != bufSz) {
    fprintf(stderr, "Error writing to file '%s'.", m_Filename);
    return -1;
  }

  return amtWritten;
}

int32 FileStream::Tell() {
  FILE *fp = m_Impl->GetFilePtr();
  if(NULL == fp) {
    return -1;
  }

  long int ret = ftell(fp);
  if(-1L == ret) {
    perror("Error opening file");
    return -1;
  }

  return ret;
}

bool FileStream::Seek(uint32 offset, ESeekPosition pos) {
  
  // We cannot seek in append mode.
  if(m_Mode == eFileMode_WriteAppend || m_Mode == eFileMode_WriteBinaryAppend)
    return false;

  FILE *fp = m_Impl->GetFilePtr();
  if(NULL == fp) return false;

  int origin = SEEK_SET;
  switch(pos) {
  default:
  case eSeekPosition_Beginning:
    // Do nothing
    break;

  case eSeekPosition_Current:
    origin = SEEK_CUR;
    break;

  case eSeekPosition_End:
    origin = SEEK_END;
    break;
  }

  if(fseek(fp, offset, origin))
    return false;

  return true;
}

void FileStream::Flush() {
  FILE *fp = m_Impl->GetFilePtr();
  if(NULL != fp) {
    fflush(fp);
  }
}
