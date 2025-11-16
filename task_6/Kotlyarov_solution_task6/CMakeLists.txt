cmake_minimum_required(VERSION 3.16)
project(daemon)

# Язык — C
set(CMAKE_C_STANDARD 11)

# Пути к заголовочным файлам
include_directories(headers)
include_directories(libs/Dynamic_array)
include_directories(libs/Get_file_size)

# Источники
set(SOURCES
    src/main.c
    src/monitor_modes.c
    src/parse_args.c
    src/parse_maps.c
    libs/Dynamic_array/Dynamic_array_funcs.c
    libs/Get_file_size/GetFileSize.c
)

# Основная цель (исполняемый файл)
add_executable(${PROJECT_NAME} ${SOURCES})

# Компилятор: GCC или Clang (для санитайзеров)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Общие флаги для Debug и Release
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

    # Debug конфигурация
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer")

    # Release конфигурация
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

    # Для отслеживания утечек памяти можно включить LeakSanitizer
    # (он включён по умолчанию в AddressSanitizer)
endif()

# Установка типа сборки по умолчанию
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Установка флагов
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# Вывод текущей сборки
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Вывод флагов для отладки
message(STATUS "C flags (Debug): ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "C flags (Release): ${CMAKE_C_FLAGS_RELEASE}")
