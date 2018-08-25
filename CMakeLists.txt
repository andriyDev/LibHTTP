cmake_minimum_required (VERSION 2.6)
project (libhttp)
set (CMAKE_BUILD_TYPE Release)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_library(libhttp STATIC $(SOURCES))
install (TARGETS libhttp DESTINATION /usr/lib)

