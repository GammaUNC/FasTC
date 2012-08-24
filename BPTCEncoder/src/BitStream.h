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
	  m_NumBytes((nBits + start_offset + 7) >> 3),
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
	int m_NextBit;
	const int m_NumBytes;
	const int m_NumBits;
	unsigned char *m_CurByte;

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
	int m_NextBit;
	const unsigned char *m_CurByte;
};
#endif //__BITSTREAM_H__