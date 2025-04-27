cmake_minimum_required(VERSION 3.10)
project(piper VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(SPA REQUIRED IMPORTED_TARGET libspa-0.2)

find_library(MATH_LIBRARY m)
include_directories(inc)

add_executable(piper
    src/main.c
    src/alsa_control.c 
)

target_include_directories(piper PRIVATE
    ${PIPEWIRE_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${SPA_INCLUDE_DIRS}
    inc
)

target_compile_definitions(piper PRIVATE
    ${PIPEWIRE_CFLAGS_OTHER}
    ${ALSA_CFLAGS_OTHER}
    ${SPA_CFLAGS_OTHER}
)

target_link_libraries(piper
    PkgConfig::PIPEWIRE
    PkgConfig::ALSA
    PkgConfig::SPA
    ${MATH_LIBRARY}
)

target_compile_options(piper PRIVATE -Wall -Wextra -Wpedantic)

set_target_properties(piper PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED YES
)

install(TARGETS piper DESTINATION bin)
