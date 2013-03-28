FasTC
=======

A **Fas**t **T**exture **C**ompressor for the BPTC (a.k.a. BC7) format. This compressor 
supports multi-threading through Boost's threading API and runs on Windows, OS X, and Linux

---

Requirements:
--------------

[CMake](http://www.cmake.org) (2.8.8)
[Boost](http://www.boost.org) (tested with v1.50 and higher)
[libpng](http://www.libpng.org/pub/png/libpng.html) (1.5.13)
[zlib](http://www.zlib.net) (1.2.5)

Installation:
--------------

FasTC uses [CMake](http://www.cmake.org/) to generate build files. The best way to do so is to
create a separate build directory for compilation:

    mkdir FasTC
    cd FasTC
    git clone git@github.com:Mokosha/FasTC.git src
    mkdir build
    cd build
    cmake ../src -DCMAKE_BUILD_TYPE=Release
    make

Once you do this you will be able to run some examples.

#### Using Visual Studio on Windows ####
Due to the C/C++ runtime requirements in Visual Studio, you must have a compiled version of 
each library that you wish to link to. In order to save time, I have uploaded various versions
of Boost, libpng, and zlib to a submodule in the source directory. Before running the steps above,
make sure to instantiate the submodule in git:

    cd FasTC/src
    git submodule init
    git submodule update

This will download all of the Release versions of the necessary libraries (which means there will
be linker errors during the build process). I have compiled versions for Visual Studio 2008, 2010,
and 2012.

Testing:
--------------

Once the compressor is built, you may test it against any images you wish as long as their dimensions
are multiples of four and they are in the png file format. If you'd like to convert from one format to
another, I suggest taking a look at [ImageMagick](http://www.imagemagick.org/script/index.php)

The quickest test will be to simply run the compressor on a PNG image:

    cd FasTC/build
    make
    CLTool/tc path/to/image.png

There are various run-time options available:

* `-t`: Specifies the number of threads to use for compression. The default is one.
* `-l`: Save an output log of various statistics during compression. This is mostly only useful for debugging.
* `-q <num>`: Use `num` steps of simulated annealing during each endpoint compression. Default is 50
* `-n <num>`: Perform `num` compressions in a row. This is good for testing metrics.
* `-a`: Use a parallel algorithm that uses Fetch-And-Add and Test-And-Set to perform mutual exclusion and avoid synchronization primitives. This algorithm is very useful when compressing a list of textures. Cannot be used with the `-j` option.
* `-j <num>`: This specifies the number of 4x4 blocks that the compressor will crunch per thread. The default is to split the image up so that each thread compresses an equal share of the image. However, for many images, certain blocks compress faster than others, and you might want a more fine grained control over how to switch between compressing different blocks.

As an example, if I wanted to test compressing a texture using no simulated annealing, 4 threads, and 32 blocks per job,
I would invoke the following command:

    CLTool/tc -q 0 -t 4 -j 32 path/to/image.png

If I wanted to compress a texture with the default amount of simulated annealing 100 times using the parallel algorithm
with atomic synchronization primitives, I would invoke the following command:

   CLTool/tc -n 100 -a path/to/image.png

