#include "ImageWriter.h"

uint32 ImageWriter::GetChannelForPixel(uint32 x, uint32 y, uint32 ch) {
	uint32 bytesPerRow = GetWidth() * 4;
	uint32 byteLocation = y * bytesPerRow + x*4 + ch;
	return m_PixelData[byteLocation];
}
