#include <stdlib.h>
#include <stdio.h>

#include "TexComp.h"

int main(int argc, char **argv) {

  if(argc != 2) {
    fprintf(stderr, "Usage: %s <imagefile>\n", argv[0]);
    exit(1);
  }

  ImageFile file (argv[1]);
  
  SCompressionSettings settings;
  CompressedImage *ci = CompressImage(file, settings);

  // Cleanup 
  delete ci;
  return 0;
}
