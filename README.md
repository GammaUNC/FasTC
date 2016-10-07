FasTC [![Build Status](https://travis-ci.org/GammaUNC/FasTC.png)](https://travis-ci.org/GammaUNC/FasTC)
=======

A <b>Fas</b>t <b>T</b>exture <b>C</b>ompressor for a variety of formats. This compressor 
supports multi-threading via native Win32 threads on Windows and pthreads on other
operating systems. It has been tested on Windows, OS X, and Ubuntu Linux.

---

Licensing:
--------------

FasTC is licensed under the Apache 2.0 license. The full license can be found
[online](http://www.apache.org/licenses/LICENSE-2.0). There are additional pieces
of FasTC that have been incorporated and made available graciously by their authors.
Each of the corresponding pieces has their license at the top of their respective
source files. These pieces include:

1. ETC1 compressor provided by Rich Geldreich, under a
[ZLIB license](https://github.com/GammaUNC/FasTC/blob/master/ETCEncoder/src/rg_etc1.h#L56).

2. DXT compressor provided by Sean Barrett, in the
[public domain](https://github.com/nothings/stb/blob/master/stb_dxt.h#L20).

3. Targa reader by Joshua S. English, under
[their own](https://github.com/GammaUNC/FasTC/blob/master/IO/third_party/tga/targa.h#L6)
license.

4. GTest as a test framework, under
[their own](https://github.com/GammaUNC/FasTC/blob/master/GTest/LICENSE)
license

Additionally, the original work for this repository was done as an
[internship project](http://software.intel.com/en-us/vcsource/samples/fast-texture-compression)
at Intel, released under a permissive MIT-like license.

Requirements:
--------------

[CMake](http://www.cmake.org) (3.1) <br>
[libpng](http://www.libpng.org/pub/png/libpng.html) (1.5.13) <br>
[zlib](http://www.zlib.net) (1.2.5)

Installation:
--------------

FasTC uses [CMake](http://www.cmake.org/) to generate build files. The best way to do so is to
create a separate build directory for compilation:

    mkdir FasTC
    cd FasTC
    git clone https://github.com/Mokosha/FasTC.git src
    mkdir build
    cd build
    cmake ../src -DCMAKE_BUILD_TYPE=Release
    make

Once you do this you will be able to run some examples.

#### Using Visual Studio on Windows ####
Due to the C/C++ runtime requirements in Visual Studio, you must have a compiled version of 
each library that you wish to link to. In order to save time, I have uploaded various versions
of libpng, and zlib to a submodule in the source directory. Before running the steps above,
make sure to instantiate the submodule in git:

    cd FasTC/src
    git submodule init
    git submodule update

This will download all of the Release versions of the necessary libraries (which means there will
be linker warnings during the build process under the Debug configuration). I have compiled
versions for Visual Studio 2008, 2010, and 2012.

Testing:
--------------

Once the compressor is built, you may test it against any images you wish as long as their dimensions
are supported by the compression format and they are in the png file format. If you'd like to convert
from one format to another, I suggest taking a look at [ImageMagick](http://www.imagemagick.org/script/index.php)

The quickest test will be to simply run the compressor on a PNG image:

    cd FasTC/build
    make
    CLTool/tc path/to/image.png
    
This will compress `image.png` into the BPTC (BC7) format using 50 steps of simulated annealing without
multithreading.

Input file format supported: PNG, PVR, TGA, KTX, ASTC

There are various run-time options available:

* `-v`: Enabled verbosity, which reports Entropy, Mean Local Entropy, and MSSIM in addition to 
compression time and PSNR.
* `-f <fmt>`: Specifies the format use for compression. `fmt` can be any one of the following:
  * [BPTC](http://www.opengl.org/registry/specs/ARB/texture_compression_bptc.txt) (**Default**)
  * [ETC1](http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt) [1]
  * [DXT1](http://www.opengl.org/registry/specs/EXT/texture_compression_s3tc.txt) [2]
  * [DXT5](http://www.opengl.org/registry/specs/EXT/texture_compression_s3tc.txt) [2]
  * [PVRTC](http://web.onetel.net.uk/~simonnihal/assorted3d/fenney03texcomp.pdf)
* `-d`: Specifies the output file. Format supported PNG, KTX
  * **Default**: `<filename>`-`<fmt>`.png
* `-nd`: Suppress decompressed output.
* `-t`: Specifies the number of threads to use for compression.
  * **Default**: 1
  * **Formats**: BPTC, ETC1, DXT1, DXT5
* `-l`: Save an output log of various statistics during compression. This is mostly only useful for
debugging.
  * **Formats**: BPTC
* `-q <num>`: Use `num` steps of simulated annealing during each endpoint compression. Default is 50.
Available only for BPTC.
  * **Default**: 50
  * **Formats**: BPTC
* `-n <num>`: Perform `num` compressions in a row. This is good for testing metrics.
  * **Default**: 1
  * **Formats**: All
* `-a`: Use a parallel algorithm that uses Fetch-And-Add and Test-And-Set to perform mutual exclusion
and avoid synchronization primitives. This algorithm is very useful when compressing a list of textures.
Cannot be used with the `-j` option.
  * **Formats**: BPTC
* `-j <num>`: This specifies the number of blocks that the compressor will request to crunch per thread. If this
flag is not specified or set to zero, the image will be split up so that each thread compresses an
equal share. However, for many images, certain blocks compress faster than others, and you might want
a more fine grained control over how to switch between compressing different blocks.
  * **Formats**: BPTC, ETC1, DXT1, DXT5

As an example, if I wanted to test compressing a texture using no simulated annealing, 4 threads, and 32 blocks per job,
I would invoke the following command:

    CLTool/tc -q 0 -t 4 -j 32 path/to/image.png

If I wanted to compress a texture with the default amount of simulated annealing 100 times using the parallel algorithm
with atomic synchronization primitives, I would invoke the following command:

    CLTool/tc -n 100 -a path/to/image.png

If I wanted to compress a texture into PVRTC, I would invoke the following command:

    CLTool/tc -f PVRTC -d path/to/image.ktx path/to/image.png

[1] Compression code courtesy of [Rich Geldreich](https://code.google.com/p/rg-etc1/) <br>
[2] Compression code courtesy of [Sean Barrett](https://github.com/nothings/stb/blob/master/stb_dxt.h).
