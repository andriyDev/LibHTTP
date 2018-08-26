cmake_minimum_required (VERSION 2.6)
project (http)
set (CMAKE_BUILD_TYPE Release)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

if (WIN32)
	set(windows)
else()
	set(posix)
endif()

add_library(http STATIC ${SOURCES})
install (TARGETS http DESTINATION /usr/lib)

