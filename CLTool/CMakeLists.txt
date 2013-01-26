
IF( MSVC )
	SET( SOURCES "src/clwin32.cpp" )
ELSE()
	SET( SOURCES "src/clunix.cpp" )
ENDIF()

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/Core/include )
INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/IO/include )

ADD_EXECUTABLE(
	tc
	${SOURCES}
)

# Make sure that if we're using boost libraries for threading then we add this linker path.
# Personally, I believe this is a bug in CMAKE but I'm not exactly sure.
IF( THREAD_API MATCHES "Boost" )
	SET_TARGET_PROPERTIES(tc PROPERTIES LINK_FLAGS "/LIBPATH:\"${Boost_LIBRARY_DIRS}\"")
ENDIF()

TARGET_LINK_LIBRARIES( tc BPTCEncoder )
TARGET_LINK_LIBRARIES( tc TexCompIO )
TARGET_LINK_LIBRARIES( tc TexCompCore )
