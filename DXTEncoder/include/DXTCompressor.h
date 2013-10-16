/*
	This code is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This code is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	Lesser General Public License for more details.
*/

#include "TexCompTypes.h"
#include "CompressionJob.h"

namespace DXTC
{
  // DXT compressor (scalar version).
  void CompressImageDXT1(const CompressionJob &);
  void CompressImageDXT5(const CompressionJob &);

  void DecompressDXT1(const DecompressionJob &);

  uint16 ColorTo565(const uint8* color);
  void EmitByte(uint8*& dest, uint8 b);
  void EmitWord(uint8*& dest, uint16 s);
  void EmitDoubleWord(uint8*& dest, uint32 i);

#if 0
  // DXT compressor (SSE2 version).
  void CompressImageDXT1SSE2(const uint8* inBuf, uint8* outBuf, uint32 width, uint32 height);
  void CompressImageDXT5SSE2(const uint8* inBuf, uint8* outBuf, uint32 width, uint32 height);
#endif
}
