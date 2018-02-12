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

# parent should define those:
# - USER_CONFIG_PATH : where to save the user_config.h
# - PRODUCT_VERSION: a version string, "1.2.3.4"
# - GIT_REVISION: the git revision hash

if(USER_CONFIG_PATH )
	string( REPLACE "." ";" PRODUCT_VERSION_COMPS ${PRODUCT_VERSION})
	list( GET PRODUCT_VERSION_COMPS 0 PRODUCT_VERSION_MAJOR )
	list( GET PRODUCT_VERSION_COMPS 1 PRODUCT_VERSION_MINOR )
	list( GET PRODUCT_VERSION_COMPS 2 PRODUCT_VERSION_PATCH )
	list( GET PRODUCT_VERSION_COMPS 3 PRODUCT_VERSION_BUILD )

	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/user_config.h.in"
					"${USER_CONFIG_PATH}/user_config.h" )
	target_include_directories( sutils PUBLIC ${USER_CONFIG_PATH} )
endif()

source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${SUTILS_HEADERS} ${SUTILS_SOURCES} )
