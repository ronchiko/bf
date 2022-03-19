cmake_minimum_required(VERSION 3.21)

project(BF-Interpeter VERSION 1.0)

include_directories(include)


FILE(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c)
LIST(FILTER SOURCES EXCLUDE REGEX "test_")
add_library(libbf STATIC ${SOURCES})

add_executable(bf main.c)
target_link_libraries(bf PRIVATE libbf)

enable_testing()
FILE(GLOB_RECURSE TESTS CONFIGURE_DEPENDS "test_*.c")

foreach(FILE ${TESTS})
	string(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "" TEST ${FILE})
	string(REPLACE "/" "." TEST ${TEST})

	set(NAME test.${TEST})

	add_executable(${NAME} ${FILE})
	target_link_libraries(${NAME} PRIVATE libbf)

	add_test(${NAME} ${NAME})	

endforeach()


