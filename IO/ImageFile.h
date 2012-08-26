#ifedef _IMAGE_FILE_H_ 
#define _IMAGE_FILE_H_ 

class ImageFile {

public:

  ImageFile(const char *filename);
  ~ImageFile();

  void GetWidth() const { return m_Width; }
  void GetHeight() const { return m_Height; }
  void GetPixels() const;

 private:
  unsigned int m_Handle;
  unsigned int m_Width;
  unsigned int m_Height;
};


#endif // _IMAGE_FILE_H_ 
