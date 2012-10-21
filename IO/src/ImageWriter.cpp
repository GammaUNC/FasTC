#include "ImageWriter.h"

uint32 ImageWriter::GetChannelForPixel(uint32 x, uint32 y, uint32 ch) {

  // Assume pixels are in block stream order, hence we would need to first find
  // the block that contains pixel (x, y) and then find the byte location for it.

  const uint32 blocksPerRow = GetWidth() / 4;
  const uint32 blockIdxX = x / 4;
  const uint32 blockIdxY = y / 4;
  const uint32 blockIdx = blockIdxY * blocksPerRow + blockIdxX;

  // Now we find the offset in the block
  const uint32 blockOffsetX = x % 4;
  const uint32 blockOffsetY = y % 4;
  const uint32 pixelOffset = blockOffsetY * 4 + blockOffsetX;

  // There are 16 pixels per block and bytes per pixel...
  uint32 dataOffset = blockIdx * 4 * 16;
  dataOffset += 4 * pixelOffset;
  dataOffset += ch;

  return m_PixelData[dataOffset];
}
