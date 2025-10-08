cmake_minimum_required(VERSION 3.14)

project(SimpleShell C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(USE_SANITIZER "Enable AddressSanitizer for memory leak detection" OFF)

if(USE_SANITIZER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
endif()

include_directories(headers)
include_directories(libs/Dynamic_array)

set(SOURCES
    src/main.c
    src/parse_cmd.c
    libs/Dynamic_array/Dynamic_array_funcs.c
)

get_filename_component(SRC_EXT "${SOURCES}" EXT)
if("${SRC_EXT}" STREQUAL ".cpp")
    enable_language(CXX)
endif()

add_executable(shell ${SOURCES})
