#ifndef _IMAGE_LOADER_H_
#define _IMAGE_LOADER_H_

enum EImageFileFormat {
  eFileFormat_PNG,

  kNumImageFileFormats
};

class ImageLoader {

 protected:
  int m_RedChannelPrecision;
  unsigned char *m_RedData;

  int m_GreenChannelPrecision;
  unsigned char *m_GreenData;

  int m_BlueChannelPrecision;
  unsigned char *m_BlueData;
  
  int m_AlphaChannelPrecision;
  unsigned char *m_AlphaData;

  const unsigned char *const m_RawData;

  ImageLoader(const unsigned char *rawData);
  virtual ~ImageLoader();

 public:
  virtual void ReadData() = 0;

  int GetRedChannelPrecision() const { return m_RedChannelPrecision; }
  unsigned char * GetRedPixelData() const { return m_RedData; }

  int GetGreenChannelPrecision() const { return m_GreenChannelPrecision; }
  unsigned char * GetGreenPixelData() const { return m_GreenData; }

  int GetBlueChannelPrecision() const { return m_BlueChannelPrecision; }
  unsigned char * GetBluePixelData() const { return m_BlueData; }

  int GetAlphaChannelPrecision() const { return m_AlphaChannelPrecision; }
  unsigned char * GetAlphaPixelData() const { return m_AlphaData; }
};

#endif // _IMAGE_LOADER_H_
