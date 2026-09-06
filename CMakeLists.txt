cmake_minimum_required(VERSION 3.21)

project(dwm)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_INSTALL_PREFIX /usr/local)

add_executable(dwm dwm.cpp drw.cpp util.cpp)

set_property(TARGET dwm PROPERTY CXX_STANDARD 20)

file(CREATE_LINK ${CMAKE_BINARY_DIR}/compile_commands.json compile_commands.json SYMBOLIC)

find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)
find_package(Fontconfig REQUIRED)

target_link_libraries(dwm PRIVATE
    X11::X11
    X11::Xft
    Freetype::Freetype
    Fontconfig::Fontconfig
  )

target_compile_definitions(dwm PUBLIC VERSION="6.8")
target_compile_definitions(dwm PUBLIC _DEFAULT_SOURCE)
target_compile_definitions(dwm PUBLIC _BSD_SOURCE)
target_compile_definitions(dwm PUBLIC _XOPEN_SOURCE=700L)

install(TARGETS dwm)
