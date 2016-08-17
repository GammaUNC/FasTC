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

// The original lisence from the code available at the following location:
// http://software.intel.com/en-us/vcsource/samples/fast-texture-compression
//
// This code has been modified significantly from the original.

//------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works
// of this software for any purpose and without fee, provided, that the above
// copyright notice and this statement appear in all copies.  Intel makes no
// representations about the suitability of this software for any purpose.  THIS
// SOFTWARE IS PROVIDED "AS IS." INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER
// INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR
// INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not assume
// any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//------------------------------------------------------------------------------

#ifndef __BASE_INCLUDE_BITSTREAM_H__
#define __BASE_INCLUDE_BITSTREAM_H__

namespace FasTC {

class BitStream {
 public:
  BitStream(unsigned char *ptr, int nBits, int start_offset) :
    m_BitsWritten(0),
    m_NumBits(nBits),
    m_CurByte(ptr),
    m_NextBit(start_offset % 8),
    done(false)
  { }

  int GetBitsWritten() const { return m_BitsWritten; }

  ~BitStream() { }
  void WriteBitsR(unsigned int val, unsigned int nBits) {
    for(unsigned int i = 0; i < nBits; i++) {
      WriteBit((val >> (nBits - i - 1)) & 1);
    }
  }

  void WriteBits(unsigned int val, unsigned int nBits) {
    for(unsigned int i = 0; i < nBits; i++) {
      WriteBit((val >> i) & 1);
    }
  }
  
 private:
  void WriteBit(int b) {
    
    if(done) return;
    
    const unsigned int mask = 1 << m_NextBit++;
    
    // clear the bit
    *m_CurByte &= ~mask;
    
    // Write the bit, if necessary
    if(b) *m_CurByte |= mask;
    
    // Next byte?
    if(m_NextBit >= 8) {
      m_CurByte += 1;
      m_NextBit = 0;
    }
    
    done = done || ++m_BitsWritten >= m_NumBits;
  }
  
  int m_BitsWritten;
  const int m_NumBits;
  unsigned char *m_CurByte;
  int m_NextBit;
  
  bool done;
};

class BitStreamReadOnly {
 public:
  BitStreamReadOnly(const unsigned char *ptr) :
    m_BitsRead(0),
    m_CurByte(ptr),
    m_NextBit(0)
  { }

  int GetBitsRead() const { return m_BitsRead; }
  
  ~BitStreamReadOnly() { }
  
  int ReadBit() {
    
    int bit = *m_CurByte >> m_NextBit++;
    while(m_NextBit >= 8) {
      m_NextBit -= 8;
      m_CurByte++;
    }
    
    m_BitsRead++;
    return bit & 1;
  }
  
  unsigned int ReadBits(unsigned int nBits) {
    unsigned int ret = 0;
    for(unsigned int i = 0; i < nBits; i++) {
      ret |= (ReadBit() & 1) << i;
    }
    return ret;
  }
  
 private:
  int m_BitsRead;
  const unsigned char *m_CurByte;
  int m_NextBit;
};

}  // namespace FasTC

#endif //__BASE_INCLUDE_BITSTREAM_H__
