# Сборка проекта
#
# Создание директории сборки:
# mkdir build && cd build
#
# Для Debug сборки (по умолчанию):
# cmake ..
# cmake --build .
#
# Для Release сборки:
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cmake --build .
#
# Для сборки с санитайзером:
# cmake .. -DUSE_SANITIZER=ON
# cmake --build .
#
# Запуск программы:
# ./duplex_pipe < input_file > output_file

cmake_minimum_required(VERSION 3.14)

project(DuplexPipe C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set(CMAKE_CONFIGURATION_TYPES Debug Release)

option(USE_SANITIZER "Enable AddressSanitizer for memory leak detection" OFF)

set(COMMON_FLAGS "-Wall -Wextra")
set(DEBUG_FLAGS "-g -O0")
set(RELEASE_FLAGS "-O3 -DNDEBUG")

if(USE_SANITIZER)
    set(SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
    set(COMMON_FLAGS "${COMMON_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS} ${RELEASE_FLAGS}")

include_directories(headers)
include_directories(libs/Rdtscp)

set(SOURCES
    src/main.c
    src/duplex_pipe.c
    libs/Rdtscp/Rdtscp.c
)

add_executable(duplex_pipe ${SOURCES})

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(copy_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        VERBATIM
    )
endif()
