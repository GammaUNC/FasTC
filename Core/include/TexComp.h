#ifndef _TEX_COMP_H_
#define _TEX_COMP_H_

#include "CompressedImage.h"

// Forward declarations
class ImageFile;
class CompressedImage;

struct SCompressionSettings {
  SCompressionSettings(); // defaults

  // The compression format for the image.
  ECompressionFormat format; 

  // The flag that requests us to use SIMD, if it is available
  bool bUseSIMD;

  // The number of threads to spawn in order to process the data
  int iNumThreads;

  // Some compression formats take a measurement of quality when
  // compressing an image. If the format supports it, this value 
  // will be used for quality purposes.
  int iQuality;

  // The number of compressions to perform. The program will compress
  // the image this many times, and then take the average of the timing.
  int iNumCompressions;

  // This setting measures the number of blocks that a thread
  // will process at any given time. If this value is zero, 
  // which is the default, the work will be divided by the
  // number of threads, and each thread will do it's job and 
  // exit.
  int iJobSize; 
};

extern bool CompressImageData(
  const unsigned char *data,
  const unsigned int dataSz,
  unsigned char *cmpData,
  const unsigned int cmpDataSz,
  const SCompressionSettings &settings
);

// A compression function format. It takes the raw data and image dimensions and 
// returns the compressed image data into outData. It is assumed that there is
// enough space allocated for outData to store the compressed data. Allocation
// is dependent on the compression format.
typedef void (* CompressionFunc)(
  const unsigned char *inData, // Raw image data
  unsigned char *outData,      // Buffer to store compressed data.
  unsigned int width,          // Image width
  unsigned int height          // Image height
);

// This function computes the Peak Signal to Noise Ratio between a 
// compressed image and a raw image.
extern double ComputePSNR(const CompressedImage &ci, const ImageFile &file);

#endif //_TEX_COMP_H_
