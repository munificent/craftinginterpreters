cmake_minimum_required(VERSION 2.8.5)
project(clox C)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_VERBOSE_MAKEFILE ON)

option(COVERAGE "Build with test coverage" OFF)

set(CLOX_SOURCES
  "${CMAKE_SOURCE_DIR}/chunk.c"
  "${CMAKE_SOURCE_DIR}/compiler.c"
  "${CMAKE_SOURCE_DIR}/debug.c"
  "${CMAKE_SOURCE_DIR}/main.c"
  "${CMAKE_SOURCE_DIR}/memory.c"
  "${CMAKE_SOURCE_DIR}/object.c"
  "${CMAKE_SOURCE_DIR}/scanner.c"
  "${CMAKE_SOURCE_DIR}/table.c"
  "${CMAKE_SOURCE_DIR}/value.c"
  "${CMAKE_SOURCE_DIR}/vm.c"
)

include_directories(
  "${CMAKE_SOURCE_DIR}"
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNAN_TAGGING -Wall -Wextra -Werror -Wno-missing-prototypes -Wno-unused-function -Wno-unused-parameter")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DDEBUG_PRINT_CODE -DDEBUG_STRESS_GC -DDEBUG_TRACE_EXECUTION -DDEBUG_TRACE_GC")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mtune=native")

set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -mtune=native")

set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -mtune=native")

set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_COVERAGE} -O0 -g -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_COVERAGE} -fprofile-arcs -ftest-coverage")

add_executable(clox ${CLOX_SOURCES})

