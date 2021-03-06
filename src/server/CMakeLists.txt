cmake_minimum_required (VERSION 3.1)
project(ev_server)

find_path(LIBEV_INCLUDE_DIR NAMES ev.h)
find_library(LIBEV_LIBRARIES NAMES ev)

if(LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)
    set(LIBEV_FOUND ON)
endif(LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)

if(LIBEV_FOUND)
    message(STATUS "Found libev includes: ${LIBEV_INCLUDE_DIR}/ev.h")
    message(STATUS "Found libev library: ${LIBEV_LIBRARIES}")
else(LIBEV_FOUND)
    message(FATAL_ERROR "Could not find libev development files")
endif (LIBEV_FOUND)

find_path(MSGPACK_INCLUDE_DIR msgpack.h)
find_library(MSGPACK_LIBRARY NAMES msgpackc PATHS)

if (MSGPACK_INCLUDE_DIR AND MSGPACK_LIBRARY)
	message(STATUS "Found MsgPack includes: ${MSGPACK_INCLUDE_DIR}/msgpack.h")
	message(STATUS "Found MsgPack library: ${MSGPACK_LIBRARY}")
else(MSGPACK_INCLUDE_DIR AND MSGPACK_LIBRARY)
	message(FATAL_ERROR "MsgPack was not found. Make sure MSGPACK_LIBRARY and MSGPACK_INCLUDE_DIR are set.")
endif(MSGPACK_INCLUDE_DIR AND MSGPACK_LIBRARY)

# find_path(BUDDY_INCLUDE_DIR NAMES memory.h)
# find_library(BUDDY_LIBRARY NAMES buddy)

# if (BUDDY_LIBRARY AND BUDDY_INCLUDE_DIR)
# 	message(STATUS "Found Buddy includes: ${BUDDY_INCLUDE_DIR}/memory.h")
# 	message(STATUS "Found Buddy library: ${BUDDY_LIBRARY}")
# else(BUDDY_LIBRARY AND BUDDY_INCLUDE_DIR)
# 	message(FATAL_ERROR "Buddy-Allocator was not found")
# endif(BUDDY_LIBRARY AND BUDDY_INCLUDE_DIR)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -ggdb -Werror -Wignored-qualifiers -Winit-self\
	-Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith \
	-Wtype-limits -Wempty-body -Wmissing-field-initializers -Wextra \
	-Wmissing-field-initializers -Wextra")

include_directories(${LIBEV_INCLUDE_DIR} ${MSGPACK_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(request_sources
	request.c
	staff.c
	log.c
	proto.c
	buffer.c
	)

set(message_sources
	message.c)

set(server_sources
	staff.c
	tcp-server.c
	)

add_library(server STATIC ${server_sources})
add_library(request STATIC ${request_sources})
add_library(message STATIC ${message_sources})

target_link_libraries(request
	${LIBEV_LIBRARIES}
	${MSGPACK_LIBRARY}
	)

target_link_libraries(server request)
