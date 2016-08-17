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

#include "FasTC/FileStream.h"

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
#define snprintf(out, outSz, fmt, ...) _snprintf_s(out, outSz, _TRUNCATE, fmt, __VA_ARGS__)
#define strncpy(dst, src, dstSz) strncpy_s(dst, dstSz, src, _TRUNCATE)
#endif

void ErrorExit(LPCSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    snprintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(CHAR),
        "%s failed with error %lu: %s", 
        lpszFunction, dw, (LPCSTR)lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

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
  DWORD dwOpenAction = OPEN_EXISTING;
  switch(mode) {
    default:
    case eFileMode_ReadBinary:
    case eFileMode_Read:
      // Do nothing.
      break;

    case eFileMode_Write:
    case eFileMode_WriteBinary:
      dwDesiredAccess = GENERIC_WRITE;
      dwOpenAction = CREATE_ALWAYS;
      break;

    case eFileMode_WriteAppend:
    case eFileMode_WriteBinaryAppend:
      dwDesiredAccess = FILE_APPEND_DATA;
      dwOpenAction = CREATE_NEW;
      break;
    }

    m_Handle = CreateFile(filename, dwDesiredAccess, 0, NULL, dwOpenAction, FILE_ATTRIBUTE_NORMAL, NULL);
    if(m_Handle == INVALID_HANDLE_VALUE) {
      ErrorExit(TEXT("CreateFile"));
    }
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
  strncpy(m_Filename, filename, kMaxFilenameSz);
  m_Filename[kMaxFilenameSz - 1] = CHAR('\0');
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

  if(m_Mode == eFileMode_Write ||
     m_Mode == eFileMode_WriteBinary ||
     m_Mode == eFileMode_WriteAppend ||
     m_Mode == eFileMode_WriteBinaryAppend
  ) {
    CHAR errStr[256];
    snprintf(errStr, 256, "Cannot read from file '%s': File opened for reading.", m_Filename);
    OutputDebugString(errStr);
    return -2;
  }

  HANDLE fp = m_Impl->GetFileHandle();
  if(INVALID_HANDLE_VALUE == fp)
    return -1;

  DWORD oldPosition = SetFilePointer(fp, 0, NULL, FILE_CURRENT);
  if(INVALID_SET_FILE_POINTER == oldPosition) {
    CHAR errStr[256];
    snprintf(errStr, 256, "Error querying the file position before reading from file '%s'(0x%lx).", m_Filename, GetLastError());
    OutputDebugString(errStr);
    return -1;
  }

  DWORD amtRead;
  BOOL success = ReadFile(fp, buf, bufSz, &amtRead, NULL);
  if(!success) {
    CHAR errStr[256];
    snprintf(errStr, 256, "Error reading from file '%s'.", m_Filename);
    OutputDebugString(errStr);
    return -1;
  }

  DWORD newPosition = SetFilePointer(fp, 0, NULL, FILE_CURRENT);
  if(INVALID_SET_FILE_POINTER == newPosition) {
    CHAR errStr[256];
    snprintf(errStr, 256, "Error querying the file position after reading from file '%s'(0x%lx).", m_Filename, GetLastError());
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
  snprintf(errStr, 256, "Cannot write to file '%s': File opened for writing.", m_Filename);
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
  snprintf(errStr, 256, "Error querying the file position before reading to file '%s'(0x%lx).", m_Filename, GetLastError());
  OutputDebugString(errStr);
  return -1;
  }

  while(!LockFile(fp, dwPos, 0, bufSz, 0)) Sleep(1);

  DWORD amtWritten;
  BOOL success = WriteFile(fp, buf, bufSz, &amtWritten, NULL);

  UnlockFile(fp, dwPos, 0, bufSz, 0);

  if(!success) {
  CHAR errStr[256];
  snprintf(errStr, 256, "Error writing to file '%s'.", m_Filename);
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
    snprintf(errStr, 256, "Error querying the file position before reading to file '%s'(0x%lx).", m_Filename, GetLastError());
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
