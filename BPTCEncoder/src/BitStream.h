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

// The original lisence from the code available at the following location:
// http://software.intel.com/en-us/vcsource/samples/fast-texture-compression
//
// This code has been modified significantly from the original.

//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

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
#endif //__BITSTREAM_H__
