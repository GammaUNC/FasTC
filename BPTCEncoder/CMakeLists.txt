INCLUDE_DIRECTORIES(${TexC_SOURCE_DIR}/BPTCEncoder/include)
INCLUDE_DIRECTORIES(${TexC_BINARY_DIR}/BPTCEncoder/include)

INCLUDE(CheckCXXSourceRuns)

SET(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
IF(CMAKE_COMPILER_IS_GNUCC)
	SET(CMAKE_REQUIRED_FLAGS -msse4.1 -E)
	CHECK_CXX_SOURCE_RUNS("
		#include <smmintrin.h>
		int main() {
		  const __m128 fv = _mm_set1_ps(1.0f);
		  const __m128 fv2 = _mm_set1_ps(2.0f);
		
		  const __m128 ans = _mm_blend_ps(fv, fv2, 2);

		  return ((int *)(&ans))[0];
		}"
		HAS_SSE_41
	)

	IF(HAS_SSE_41)
		SET(CMAKE_REQUIRED_FLAGS -msse4.2)
		CHECK_CXX_SOURCE_RUNS("
			#include <smmintrin.h>
			int main() {
				const unsigned int testMe = 5;
				return _mm_popcnt_u32(testMe);
			}"
			HAS_SSE_POPCNT
		)
	ENDIF(HAS_SSE_41)

ELSEIF(MSVC)
#!FIXME!
ENDIF()
SET(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})

CONFIGURE_FILE(
	"config/BC7Config.h.in"
	"include/BC7Config.h"
)

IF(CMAKE_COMPILER_IS_GNUCC)
	ADD_DEFINITIONS(-fasm-blocks)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

SET( HEADERS
	src/BC7CompressionMode.h
	src/BC7IntTypes.h
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

ADD_LIBRARY( BPTCEncoder
	${HEADERS}
	${SOURCES}
)
