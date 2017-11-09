cmake_minimum_required( VERSION 3.8 )
project( sutils_tests )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

set( TEST_RUNNER ../src/su/tests/simple_tests_runner.cpp
				../src/su/tests/simple_tests.h )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../src )

file( GLOB ALL_TESTS "../src/su/*/tests/*_tests.cpp" )

file( GLOB SUTILS_HEADERS
		"../src/su/base/*.h"
		"../src/su/concurrency/*.h"
		"../src/su/containers/*.h"
		"../src/su/files/*.h"
		"../src/su/json/*.h"
		"../src/su/log/*.h"
		"../src/su/miscs/*.h"
		"../src/su/parsers/*.h"
		"../src/su/streams/*.h"
		"../src/su/strings/*.h"
)
file( GLOB SUTILS_SOURCES
		"../src/su/base/*.cpp"
		"../src/su/concurrency/*.cpp"
		"../src/su/containers/*.cpp"
		"../src/su/files/*.cpp"
		"../src/su/json/*.cpp"
		"../src/su/log/*.cpp"
		"../src/su/miscs/*.cpp"
		"../src/su/parsers/*.cpp"
		"../src/su/streams/*.cpp"
		"../src/su/strings/*.cpp"
)

add_executable ( sutils_tests
					${TEST_RUNNER}
					${ALL_TESTS}
					
					${SUTILS_HEADERS}
					${SUTILS_SOURCES} )

if(APPLE)
	add_custom_command( TARGET sutils_tests
						POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy_directory rsrc "$<TARGET_FILE_DIR:sutils_tests>"
						WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
						COMMENT "copy rsrc" )
else()
	add_custom_command( TARGET sutils_tests
						POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E copy_directory rsrc "$<TARGET_FILE_DIR:sutils_tests>/Resources"
						WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
						COMMENT "copy rsrc" )
endif()


source_group( "sutils" FILES
					${SUTILS_HEADERS}
					${SUTILS_SOURCES} )
source_group( "tests" FILES
					${TEST_RUNNER}
					${ALL_TESTS} )

add_definitions( -DENABLE_SIMPLE_TESTS )

add_definitions( -DPRODUCT_VERSION_FULL=1,2,3,4 )
execute_process( COMMAND svn info --show-item revision
					WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
					OUTPUT_VARIABLE SVN_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE )
if(SVN_REVISION)
	add_definitions( -DSVN_REVISION=${SVN_REVISION} )
else()
	message("no svn revision")
endif()

execute_process( COMMAND git rev-parse HEAD
					WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
					OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE )
if(GIT_REVISION)
	add_definitions( -DGIT_REVISION=${GIT_REVISION} )
else()
	message("no git revision")
endif()

find_library( EXPAT_LIBRARY expat )
if(EXPAT_LIBRARY)
	target_link_libraries( sutils_tests ${EXPAT_LIBRARY} )
	add_definitions( -DHAS_EXPAT )
endif()

find_library( SQLITE3_LIBRARY sqlite3 )
if(SQLITE3_LIBRARY)
	target_link_libraries( sutils_tests ${SQLITE3_LIBRARY} )
endif()

find_package( Threads )
target_link_libraries( sutils_tests ${CMAKE_THREAD_LIBS_INIT} )

if(APPLE)
	find_library( COREFOUNDATION_LIBRARY CoreFoundation )
	target_link_libraries( sutils_tests
							${COREFOUNDATION_LIBRARY} )
elseif(UNIX)
	find_library( UUID_LIBRARY uuid )
	find_package( ICU REQUIRED COMPONENTS uc )
	target_link_libraries( sutils_tests
							${UUID_LIBRARY} ICU::uc )
else()
	add_definitions( -D_SCL_SECURE_NO_WARNINGS )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS )
	find_library( RPCRT4_LIBRARY rpcrt4 )
	target_link_libraries( sutils_tests ${RPCRT4_LIBRARY} )
endif()

enable_testing()

foreach( test ${ALL_TESTS} )
	get_filename_component( test ${test} NAME_WE )
	add_test( NAME ${test}
				COMMAND sutils_tests --run ${test}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
endforeach()

