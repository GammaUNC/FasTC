#include "FileStream.h"

#include <Windows.h>
#include <stdio.h>
#include <assert.h>

class FileStreamImpl {
  
private:
  // Track the number of references to this filestream so
  // that we know whether or not we need to close it when
  // the object gets destroyed.
  uint32 m_ReferenceCount;

  HANDLE m_Handle;

public:
  FileStreamImpl(const CHAR *filename, EFileMode mode) 
    : m_ReferenceCount(1)
  {

	DWORD dwDesiredAccess = GENERIC_READ;
	DWORD dwShareMode = 0;
    switch(mode) {
    default:
    case eFileMode_ReadBinary:
    case eFileMode_Read:
      // Do nothing.
      break;

    case eFileMode_Write:
    case eFileMode_WriteBinary:
      dwDesiredAccess = GENERIC_WRITE;
      break;

    case eFileMode_WriteAppend:
    case eFileMode_WriteBinaryAppend:
		dwDesiredAccess = FILE_APPEND_DATA;
      break;
    }

    m_Handle = CreateFile(filename, dwDesiredAccess, dwShareMode, NULL, 0, FILE_ATTRIBUTE_NORMAL, NULL);
	assert(m_Handle != INVALID_HANDLE_VALUE);
  }

  ~FileStreamImpl() {
    CloseHandle(m_Handle);
  }

  void IncreaseReferenceCount() { m_ReferenceCount++; }
  void DecreaseReferenceCount() { m_ReferenceCount--; }

  uint32 GetReferenceCount() { return m_ReferenceCount; }

  HANDLE GetFileHandle() const { return m_Handle; }
};

FileStream::FileStream(const CHAR *filename, EFileMode mode)
  : m_Impl(new FileStreamImpl(filename, mode))
  , m_Mode(mode)
{
  strncpy_s(m_Filename, filename, kMaxFilenameSz);
  m_Filename[kMaxFilenameSz - 1] = CHAR('\0');
}
    
FileStream::FileStream(const FileStream &other)
  : m_Impl(other.m_Impl)
  , m_Mode(other.m_Mode)
{
  m_Impl->IncreaseReferenceCount();
  strncpy_s(m_Filename, other.m_Filename, kMaxFilenameSz);
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
  strncpy_s(m_Filename, other.m_Filename, kMaxFilenameSz); 

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
    CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Cannot read from file '%s': File opened for reading.", m_Filename);
	OutputDebugString(errStr);
    return -2;
  }

  HANDLE fp = m_Impl->GetFileHandle();
  if(INVALID_HANDLE_VALUE == fp)
    return -1;

  DWORD oldPosition = SetFilePointer(fp, 0, NULL, FILE_CURRENT);
  if(INVALID_SET_FILE_POINTER == oldPosition) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Error querying the file position before reading from file '%s'(0x%x).", m_Filename, GetLastError());
	OutputDebugString(errStr);
	return -1;
  }

  DWORD amtRead;
  BOOL success = ReadFile(fp, buf, bufSz, &amtRead, NULL);
  if(!success) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Error reading from file '%s'.", m_Filename);
	OutputDebugString(errStr);
    return -1;
  }

  DWORD newPosition = SetFilePointer(fp, 0, NULL, FILE_CURRENT);
  if(INVALID_SET_FILE_POINTER == newPosition) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Error querying the file position after reading from file '%s'(0x%x).", m_Filename, GetLastError());
	OutputDebugString(errStr);
	return -1;
  }

  return newPosition - oldPosition;
}

int32 FileStream::Write(const uint8 *buf, uint32 bufSz) {
  if(
     m_Mode == eFileMode_Read ||
     m_Mode == eFileMode_ReadBinary
  ) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Cannot write to file '%s': File opened for writing.", m_Filename);
	OutputDebugString(errStr);
    return -2;
  }

  HANDLE fp = m_Impl->GetFileHandle();
  if(NULL == fp)
    return -1;

  DWORD dwPos;
  if(m_Mode == eFileMode_WriteBinaryAppend || m_Mode == eFileMode_WriteAppend) {
    dwPos = SetFilePointer(fp, 0, NULL, FILE_END);
  }
  else {
    dwPos = SetFilePointer(fp, 0, NULL, FILE_CURRENT);
  }

  if(INVALID_SET_FILE_POINTER == dwPos) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Error querying the file position before reading to file '%s'(0x%x).", m_Filename, GetLastError());
	OutputDebugString(errStr);
	return -1;
  }

  while(!LockFile(fp, dwPos, 0, bufSz, 0)) Sleep(1);

  DWORD amtWritten;
  BOOL success = WriteFile(fp, buf, bufSz, &amtWritten, NULL);

  UnlockFile(fp, dwPos, 0, bufSz, 0);

  if(!success) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Error writing to file '%s'.", m_Filename);
	OutputDebugString(errStr);
    return -1;
  }

  return amtWritten;
}

int32 FileStream::Tell() {
  HANDLE fp = m_Impl->GetFileHandle();
  if(NULL == fp) {
    return -1;
  }

  DWORD pos =  SetFilePointer(fp, 0, NULL, FILE_CURRENT);
  if(INVALID_SET_FILE_POINTER == pos) {
	CHAR errStr[256];
	_sntprintf_s(errStr, 256, "Error querying the file position before reading to file '%s'(0x%x).", m_Filename, GetLastError());
	OutputDebugString(errStr);
	return -1;
  }

  return pos;
}

bool FileStream::Seek(uint32 offset, ESeekPosition pos) {
  
  // We cannot seek in append mode.
  if(m_Mode == eFileMode_WriteAppend || m_Mode == eFileMode_WriteBinaryAppend)
    return false;

  HANDLE fp = m_Impl->GetFileHandle();
  if(NULL == fp) return false;

  DWORD origin = FILE_BEGIN;
  switch(pos) {
  default:
  case eSeekPosition_Beginning:
    // Do nothing
    break;

  case eSeekPosition_Current:
    origin = FILE_CURRENT;
    break;

  case eSeekPosition_End:
    origin = FILE_END;
    break;
  }

  if(SetFilePointer(fp, offset, NULL, origin) == INVALID_SET_FILE_POINTER)
    return false;

  return true;
}

void FileStream::Flush() {
  HANDLE fp = m_Impl->GetFileHandle();
  if(NULL == fp) return;

  FlushFileBuffers(fp);
}
