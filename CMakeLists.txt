cmake_minimum_required( VERSION 3.8 )
project( sutils )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file( GLOB_RECURSE SUTILS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" )
file( GLOB_RECURSE SUTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
list( REMOVE_ITEM SUTILS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/su/tests/simple_tests_runner.cpp" )

add_library( sutils STATIC
					${SUTILS_HEADERS}
					${SUTILS_SOURCES} )

target_include_directories( sutils PUBLIC src )

find_library( EXPAT_LIBRARY expat )
if(EXPAT_LIBRARY)
	target_link_libraries( sutils ${EXPAT_LIBRARY} )
endif()

find_package( Threads )
target_link_libraries( sutils ${CMAKE_THREAD_LIBS_INIT} )

if(APPLE)
	find_library( COREFOUNDATION_LIBRARY CoreFoundation )
	target_link_libraries( sutils
							${COREFOUNDATION_LIBRARY} )
elseif(UNIX)
	find_library( UUID_LIBRARY uuid )
	find_package( ICU REQUIRED COMPONENTS uc )
	target_link_libraries( sutils
							${UUID_LIBRARY} ICU::uc )
else()
	target_compile_definitions( -D_SCL_SECURE_NO_WARNINGS )
	target_compile_definitions( -D_CRT_SECURE_NO_WARNINGS )
	find_library( RPCRT4_LIBRARY rpcrt4 )
	target_link_libraries( sutils ${RPCRT4_LIBRARY} )
endif()

# parent should define those:
# - PRODUCT_VERSION: a version string, "1.2.3.4"
# - GIT_REVISION: the git revision hash
# - COMPILETIME_LOG_MASK
#

if( NOT DEFINED PRODUCT_VERSION )
	message( "PRODUCT_VERSION is not defined" )
endif()
if( NOT DEFINED  GIT_REVISION )
	message( "GIT_REVISION is not defined" )
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/user_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/user_config.h" )
target_include_directories( sutils PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include" )

source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SUTILS_HEADERS} ${SUTILS_SOURCES} )
