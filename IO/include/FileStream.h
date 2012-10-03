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
  int64 Tell();
  
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

  EFileMode m_Mode;

  // Platform specific implementation 
  FileStreamImpl *m_Impl;

  static const uint32 kMaxFilenameSz = 256;
  CHAR m_Filename[kMaxFilenameSz];
};

#endif // __FILE_STREAM_H__
