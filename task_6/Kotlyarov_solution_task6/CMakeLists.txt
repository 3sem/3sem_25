cmake_minimum_required(VERSION 3.16)
project(daemon)

set(CMAKE_C_STANDARD 11)

include_directories(headers)
include_directories(libs/Dynamic_array)
include_directories(libs/Get_file_size)

set(SOURCES
    src/main.c
    src/monitor_modes.c
    src/parse_args.c
    src/parse_maps.c
    src/backup_funcs.c
    src/maps_diff.c
    src/command_handler.c
    libs/Dynamic_array/Dynamic_array_funcs.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

message(STATUS "C flags (Debug): ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "C flags (Release): ${CMAKE_C_FLAGS_RELEASE}")
