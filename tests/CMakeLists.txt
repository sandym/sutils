cmake_minimum_required( VERSION 3.8 )
project( sutils_tests )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set( TEST_RUNNER ../src/su/tests/simple_tests_runner.cpp )

# configure
execute_process( COMMAND git rev-parse HEAD
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
				OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE )

set( PRODUCT_VERSION "0.2.3.4" )
add_definitions( -DENABLE_SIMPLE_TESTS )

set( USER_CONFIG_PATH "${CMAKE_BINARY_DIR}/user_config" )
add_subdirectory( .. lib )

file( GLOB ALL_TESTS "*_tests.cpp" )

add_executable ( sutils_tests
					${TEST_RUNNER}
					${ALL_TESTS} )

target_link_libraries( sutils_tests sutils )

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


source_group( "tests" FILES
					${TEST_RUNNER}
					${ALL_TESTS} )

find_library( EXPAT_LIBRARY expat )
if(EXPAT_LIBRARY)
	target_link_libraries( sutils_tests ${EXPAT_LIBRARY} )
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

