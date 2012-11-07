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

int64 FileStream::Tell() {
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
