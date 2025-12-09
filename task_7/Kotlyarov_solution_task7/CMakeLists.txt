cmake_minimum_required(VERSION 3.16)
project(task7_ioserver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(HEADERS_DIR headers)
set(SRC_DIR src)
set(LIBS_DIR libs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror")

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckCCompilerFlag)
    
    check_c_compiler_flag("-fsanitize=address" HAS_ASAN)
    check_c_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
    
    if(HAS_ASAN AND HAS_UBSAN)
        set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
    elseif(HAS_UBSAN)
        set(SANITIZER_FLAGS "-fsanitize=undefined")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZER_FLAGS}")
    endif()
endif()

include_directories(${HEADERS_DIR})
include_directories(${LIBS_DIR}/Check_flags)

set(CLIENT_SOURCES
    ${SRC_DIR}/client.c
    ${SRC_DIR}/client_funcs.c
    ${SRC_DIR}/fifo_utils.c
    ${LIBS_DIR}/Check_flags/Check_r_w_flags.c
)

set(SERVER_SOURCES
    ${SRC_DIR}/server.c
    ${SRC_DIR}/server_funcs.c
    ${SRC_DIR}/fifo_utils.c
    ${SRC_DIR}/thread_utils.c
)

add_executable(client ${CLIENT_SOURCES})
add_executable(server ${SERVER_SOURCES})

# ДОБАВИТЬ ЗДЕСЬ - после add_executable, перед target_link_libraries
target_compile_definitions(client PRIVATE _POSIX_C_SOURCE=200809L)
target_compile_definitions(server PRIVATE _POSIX_C_SOURCE=200809L)

target_link_libraries(client pthread)
target_link_libraries(server pthread)

foreach(target client server)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()

# Тестовая программа
add_executable(test_multiple_clients
    tests/test_multiple_clients.c
)

target_link_libraries(test_multiple_clients pthread)

# Кастомная цель для запуска тестов
add_custom_target(run_test
    COMMAND ${CMAKE_BINARY_DIR}/test_multiple_clients
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS client server test_multiple_clients
    COMMENT "Running IO Server stress test"
)

# Добавляем тест в CTest
enable_testing()
add_test(NAME stress_test COMMAND ${CMAKE_BINARY_DIR}/test_multiple_clients)
