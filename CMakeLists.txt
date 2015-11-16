cmake_minimum_required(VERSION 2.8.7)
project(pone)

enable_testing()

include_directories(3rd/uthash/include/ 3rd/linenoise/ 3rd/rockre/include 3rd/pvip/src/ src/)

SET(CMAKE_C_FLAGS   "-fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -pthread -std=c99 -Wall -W -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function -g")
# -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
SET(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS} ")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
SET(CMAKE_BUILD_TYPE Release)

set(WERROR "0" CACHE BOOL "enable -Werror")
if (WERROR)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

set(THREAD_DEBUG "0" CACHE BOOL "enable thread debugging")
if (THREAD_DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DTHREAD_DEBUG")
endif()

set(EXC_DEBUG "0" CACHE BOOL "enable EXC debugging")
if (EXC_DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DEXC_DEBUG")
endif()

set(GC_DEBUG "0" CACHE BOOL "enable GC debugging")
if (GC_DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DGC_DEBUG")
endif()

set(WORLD_DEBUG "0" CACHE BOOL "enable world tracing")
if (WORLD_DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DWORLD_DEBUG")
endif()

set(GC_RD_LOCK_DEBUG "0" CACHE BOOL "enable GC rd lock tracing")
if (GC_RD_LOCK_DEBUG)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DGC_RD_LOCK_DEBUG")
endif()

set(NO_REUSE "0" CACHE BOOL "do not reuse free'd value")
if (NO_REUSE)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DNO_REUSE")
endif()

set(STRESS_GC "0" CACHE BOOL "stress gc")
if (STRESS_GC)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DSTRESS_GC")
endif()


SET(serial "0.0.1")
SET(soserial "1")

SET(SRCS
    src/channel.c src/gc.c src/pair.c src/obj.c src/class.c src/alloc.c src/array.c src/bool.c src/builtin.c src/code.c src/hash.c src/int.c src/nil.c src/num.c src/op.c src/pone.c src/scope.c src/str.c src/world.c src/universe.c src/iter.c src/exc.c src/range.c src/mu.c src/any.c src/cool.c src/socket.c src/re.c src/thread.c src/signal.c
    )

add_executable(pone src/compiler/main.c 3rd/linenoise/linenoise.c)
target_link_libraries(pone libpvip libpone rockre-static dl pthread)
set_target_properties(pone
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin/")

ADD_LIBRARY(libpone STATIC ${SRCS})
set_target_properties(libpone
    PROPERTIES
    OUTPUT_NAME "pone")

ADD_LIBRARY(linenoise STATIC 3rd/linenoise/linenoise.c)
set_source_files_properties(3rd/linenoise/linenoise.c PROPERTIES COMPILE_FLAGS "-g -Wno-implicit-function-declaration")

add_test(NAME prove COMMAND prove t xt)

add_subdirectory(3rd/pvip)
add_subdirectory(3rd/rockre)

