# Сборка Debug: mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
# Запуск Debug: ./task5-signals <input_file> <output_file>
# Сборка Release: cmake -DCMAKE_BUILD_TYPE=Release .. && make
cmake_minimum_required(VERSION 3.10)
project(task5-signals VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.c
    src/handlers.c
)

add_executable(task5-signals ${SOURCES})

target_include_directories(task5-signals PRIVATE
    ${CMAKE_SOURCE_DIR}/headers
)

set(COMMON_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -g
)

target_compile_options(task5-signals PRIVATE ${COMMON_COMPILE_OPTIONS})

option(BUILD_WITH_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer for Debug builds" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(task5-signals PRIVATE DEBUG=1)
    if(BUILD_WITH_SANITIZE)
        target_compile_options(task5-signals PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(task5-signals PRIVATE
            -fsanitize=address,undefined
        )
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(task5-signals PRIVATE
        -O3
    )
endif()

