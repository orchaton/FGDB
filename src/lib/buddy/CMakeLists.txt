cmake_minimum_required (VERSION 3.1)
project(buddy_allocator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self\
	-Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith \
	-Wtype-limits -Wempty-body -Wmissing-field-initializers -Wextra -pedantic \
	-Wno-pointer-to-int-cast -Wno-int-conversion")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_STANDARD 99)

include_directories(include)
add_library(buddy STATIC memory.c)
