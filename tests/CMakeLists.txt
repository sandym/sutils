cmake_minimum_required( VERSION 3.5 )
project( sutils_tests )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set( TEST_RUNNER simple_test_runner.cpp simple_tester.h )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../source )

file( GLOB ALL_TESTS
    "*_tests.cpp"
)

file( GLOB SUTILS_HEADERS
    "../source/*.h"
)
file( GLOB SUTILS_SOURCES
    "../source/*.cpp"
)

add_executable ( sutils_tests
					${TEST_RUNNER}
					${ALL_TESTS}
					
					${SUTILS_HEADERS}
					${SUTILS_SOURCES}
				)

source_group( "sutils" FILES
					${SUTILS_HEADERS}
					${SUTILS_SOURCES}
			 )
source_group( "tests" FILES
					${TEST_RUNNER}

					${ALL_TESTS}
			)

add_definitions( -DBUILD_VERSION=1,2,3,4 )
execute_process( COMMAND svn info --show-item revision
					OUTPUT_VARIABLE SVN_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( -DSVN_REVISION=${SVN_REVISION} )

execute_process( COMMAND git rev-parse HEAD
					OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( -DGIT_REVISION=${GIT_REVISION} )

find_library( EXPAT_LIBRARY expat )
if(EXPAT_LIBRARY)
	target_link_libraries( sutils_tests ${EXPAT_LIBRARY} )
else()
	add_definitions( -DSU_NO_EXPAT )
endif()

find_package( Threads )
target_link_libraries( sutils_tests ${CMAKE_THREAD_LIBS_INIT} )

if(APPLE)
	find_library( COREFOUNDATION_LIBRARY CoreFoundation )
	target_link_libraries( sutils_tests
							${COREFOUNDATION_LIBRARY} )
elseif(UNIX)
	find_library( UUID_LIBRARY uuid )
	target_link_libraries( sutils_tests
							${UUID_LIBRARY} )
else()
	add_definitions( -D_SCL_SECURE_NO_WARNINGS )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS )
	find_library( RPCRT4_LIBRARY rpcrt4 )
	target_link_libraries( sutils_tests ${RPCRT4_LIBRARY} )
endif()

