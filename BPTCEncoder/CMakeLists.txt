# FasTC
# Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its documentation for educational, 
# research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
# provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
# in all copies.
#
# Permission to incorporate this software into commercial products may be obtained by contacting the 
# authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
#
# This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
# The software program and documentation are supplied "as is," without any accompanying services from the 
# University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
# and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
# end-user understands that the program was developed for research purposes and is advised not to rely 
# exclusively on the program for any reason.
#
# IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
# USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
# AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
# STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
# OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
# ENHANCEMENTS, OR MODIFICATIONS.
#
# Please send all BUG REPORTS to <pavel@cs.unc.edu>.
#
# The authors may be contacted via:
#
# Pavel Krajcevski
# Dept of Computer Science
# 201 S Columbia St
# Frederick P. Brooks, Jr. Computer Science Bldg
# Chapel Hill, NC 27599-3175
# USA
# 
# <http://gamma.cs.unc.edu/FasTC/>

INCLUDE_DIRECTORIES(${TexC_SOURCE_DIR}/Core/include)

INCLUDE_DIRECTORIES(${TexC_SOURCE_DIR}/BPTCEncoder/include)
INCLUDE_DIRECTORIES(${TexC_BINARY_DIR}/BPTCEncoder/include)

INCLUDE(CheckCXXSourceRuns)

#SET(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
#IF(CMAKE_COMPILER_IS_GNUCC)
#
#        # Test whether or not the compiler allows inline
#	# assmbly...
#	CHECK_CXX_SOURCE_RUNS("
#            \#ifdef _MSC_VER
#             int main() {
#             int x = 1, y = 0;
#               __asm {
#                mov eax, x
#                mov y, eax
#               }
#             return !y;
#             }
#            \#else
#             int main() {
#             int x = 1, y = 0;
#             __asm__ (\"movl %1, %%eax;\"
#                      \"movl %%eax, %0;\"
#                       :\"=r\"(y)        /* output */
#                       :\"r\"(x)         /* input */
#                       :\"%eax\"         /* clobbered register */
#             ); 
#
#             return !y;
#             }
#            \#endif"
#            HAS_INLINE_ASSEMBLY
#        )
#
#        # If the compiler doesn't allow it, then try with a
#        # compiler flag...
#	IF( NOT HAS_INLINE_ASSEMBLY )
#          SET(CMAKE_REQUIRED_FLAGS -fasm-blocks)
#          CHECK_CXX_SOURCE_RUNS("
#            \#ifdef _MSC_VER
#             int main() {
#             int x = 1, y = 0;
#               __asm {
#                mov eax, x
#                mov y, eax
#               }
#             return !y;
#             }
#            \#else
#             int main() {
#             int x = 1, y = 0;
#             __asm__ (\"movl %1, %%eax;\"
#                      \"movl %%eax, %0;\"
#                       :\"=r\"(y)        /* output */
#                       :\"r\"(x)         /* input */
#                       :\"%eax\"         /* clobbered register */
#             ); 
#
#             return !y;
#             }
#            \#endif"
#            HAS_INLINE_ASSEMBLY_WITH_FLAGS
#          )
#        ENDIF()
#
#	SET(CMAKE_REQUIRED_FLAGS -msse4.1)
#	CHECK_CXX_SOURCE_RUNS("
#		#include <smmintrin.h>
#		int main() {
#		  const __m128 fv = _mm_set1_ps(1.0f);
#		  const __m128 fv2 = _mm_set1_ps(2.0f);
#		
#		  const __m128 ans = _mm_blend_ps(fv, fv2, 2);
#
#		  return ((int *)(&ans))[0];
#		}"
#		HAS_SSE_41
#	)
#
#	IF(HAS_SSE_41)
#		SET(CMAKE_REQUIRED_FLAGS -msse4.2)
#		CHECK_CXX_SOURCE_RUNS("
#			#include <smmintrin.h>
#			int main() {
#				const unsigned int testMe = 5;
#				return !(2 == _mm_popcnt_u32(testMe));
#			}"
#			HAS_SSE_POPCNT
#		)
#	ENDIF(HAS_SSE_41)
#
#ELSEIF(MSVC)
##!FIXME!
#ENDIF()
#SET(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})

IF( NOT HAS_INLINE_ASSEMBLY AND NOT HAS_INLINE_ASSEMBLY_WITH_FLAGS )
  SET( NO_INLINE_ASSEMBLY true )
ENDIF()

CONFIGURE_FILE(
	"config/BC7Config.h.in"
	"include/BC7Config.h"
)

SET( HEADERS
	src/BC7CompressionMode.h
	src/BitStream.h
	src/RGBAEndpoints.h
)

SET( SOURCES
	src/BC7Compressor.cpp
	src/RGBAEndpoints.cpp
)

IF( HAS_SSE_41 ) 

	IF ( HAS_SSE_POPCNT )
		IF( MSVC )
			ADD_DEFINITIONS( /arch:SSE4.2 )
		ELSE() #Assume GCC
			ADD_DEFINITIONS( -msse4.2 )
		ENDIF()
	ELSE()
		IF( MSVC )
			ADD_DEFINITIONS( /arch:SSE4.1 )
		ELSE() #Assume GCC
			ADD_DEFINITIONS( -msse4.1 )
		ENDIF()
	ENDIF()

	SET( HEADERS
		${HEADERS}
		src/RGBAEndpointsSIMD.h
		src/BC7CompressionModeSIMD.h
	)

	SET( SOURCES
		${SOURCES}
		src/BC7CompressorSIMD.cpp
		src/RGBAEndpointsSIMD.cpp
	)
ENDIF( HAS_SSE_41 )

IF( HAS_INLINE_ASSEMBLY_WITH_FLAGS )
  IF( MSVC )
    # !FIXME!
  ELSE()
    ADD_DEFINITIONS( -fasm-blocks )
  ENDIF()
ENDIF()

ADD_LIBRARY( BPTCEncoder
	${HEADERS}
	${SOURCES}
)
