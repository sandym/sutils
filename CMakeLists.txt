cmake_minimum_required( VERSION 3.5 )
project( sutils )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/source )

file( GLOB SUTILS_HEADERS
    "source/*.h"
)
file( GLOB SUTILS_SOURCES
    "source/*.cpp"
)

add_library( sutils STATIC
					${SUTILS_HEADERS}
					${SUTILS_SOURCES}
				)

source_group( "sutils" FILES
					${SUTILS_HEADERS}
					${SUTILS_SOURCES}
			 )

find_library( EXPAT_LIBRARY expat )
if(EXPAT_LIBRARY)
	target_link_libraries( sutils ${EXPAT_LIBRARY} )
	add_definitions( -DHAS_EXPAT )
endif()

find_package( Threads )
target_link_libraries( sutils ${CMAKE_THREAD_LIBS_INIT} )

if(APPLE)
	find_library( COREFOUNDATION_LIBRARY CoreFoundation )
	target_link_libraries( sutils
							${COREFOUNDATION_LIBRARY} )
elseif(UNIX)
	find_library( UUID_LIBRARY uuid )
	target_link_libraries( sutils
							${UUID_LIBRARY} )
else()
	add_definitions( -D_SCL_SECURE_NO_WARNINGS )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS )
	find_library( RPCRT4_LIBRARY rpcrt4 )
	target_link_libraries( sutils ${RPCRT4_LIBRARY} )
endif()

