cmake_minimum_required(VERSION 3.16)
project("$ENV{PROJECT_NAME}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (DEBUG_FLAGS -std=c++20 -ggdb3 -fsanitize=address,alignment -Wall -Wextra -Weffc++ -Wc++14-compat
				 -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconversion -Wctor-dtor-privacy
				 -Wempty-body -Wfloat-equal -Wno-format-nonliteral -Wformat-security -Wformat=2 -Winline
				 -Wnon-virtual-dtor -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls
				 -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-overflow=2 -Wsuggest-override -Wswitch-default
				 -Wundef -Wunreachable-code -Wunused -Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing
				 -Wno-old-style-cast -Wno-varargs -Wno-unused-parameter -fcheck-new -fsized-deallocation -fstack-protector
				 -fstrict-overflow -fPIC -fPIE -fno-omit-frame-pointer -fPIE -Werror=vla -Wno-write-strings)
set (RELEASE_FLAGS -std=c++20)

find_program(IWYU_PATH NAMES include-what-you-use iwyu)

if(IWYU_PATH)
    message(STATUS "Found include-what-you-use: ${IWYU_PATH}")

    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_PATH}"
        "-Xiwyu;--verbose=1;"
        "-Xiwyu;--no_comments;"
    )
else()
    message(STATUS "include-what-you-use not found, continuing without it")
endif()

file(GLOB SOURCES "src/*.cpp" "src/*.c")
file(GLOB HEADERS "include/*.hpp" "include/*.h")
foreach(src_file ${SOURCES})
    get_filename_component(target_name ${src_file} NAME_WE)
    add_executable(${target_name} ${src_file})
    target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DEBUG_FLAGS}>)
    target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DEBUG_FLAGS}>)
    target_include_directories(${target_name} PRIVATE include)
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:${DEBUG_FLAGS}>)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:${RELEASE_FLAGS}>)

target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:${DEBUG_FLAGS}>)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:${RELEASE_FLAGS}>)
