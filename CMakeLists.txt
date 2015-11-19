cmake_minimum_required(VERSION 2.8.7)
project(pone)

INCLUDE(CheckSymbolExists)

enable_testing()

include_directories(3rd/uthash/include/ 3rd/linenoise/ 3rd/onigmo/ 3rd/pvip/src/ src/ include)

list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(accept4 sys/socket.h HAVE_ACCEPT4)
check_symbol_exists(SOCK_CLOEXEC sys/socket.h HAVE_SOCK_CLOEXEC)
configure_file(include/pone_config.h.in include/pone_config.h)

SET(CMAKE_C_FLAGS   "-fPIC -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -pthread -std=c99 -Wall -W -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function -g")
# -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
SET(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS} ")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
message("-- type: ${CMAKE_BUILD_TYPE}")

set(WERROR "0" CACHE BOOL "enable -Werror")
if (WERROR)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

set(DEBUG_THREAD "0" CACHE BOOL "enable thread debugging")
if (DEBUG_THREAD)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_THREAD")
endif()

set(DEBUG_EXC "0" CACHE BOOL "enable EXC debugging")
if (DEBUG_EXC)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_EXC")
endif()

set(DEBUG_GC "0" CACHE BOOL "enable GC debugging")
if (DEBUG_GC)
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_GC")
endif()

set(DEBUG_WORLD "0" CACHE BOOL "enable world tracing")
if (DEBUG_WORLD)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG_WORLD")
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
    src/os.c src/arg.c src/errno.c src/opaque.c src/channel.c src/gc.c src/pair.c src/obj.c src/class.c src/alloc.c src/array.c src/bool.c src/builtin.c src/code.c src/hash.c src/int.c src/nil.c src/num.c src/op.c src/pone.c src/scope.c src/str.c src/world.c src/universe.c src/iter.c src/exc.c src/range.c src/mu.c src/any.c src/cool.c src/socket.c src/re.c src/thread.c src/signal.c
    src/pvip_node.c src/pvip_string.c gen.pone.y.c src/gen.node.c
    )

add_executable(pone src/compiler/main.c 3rd/linenoise/linenoise.c)
target_link_libraries(pone m libpone onigmo dl pthread)
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

add_executable(greg 3rd/greg/greg.c 3rd/greg/tree.c 3rd/greg/compile.c)
set_target_properties(greg PROPERTIES COMPILE_FLAGS "-g -Wno-sign-compare")

add_custom_command(
    OUTPUT gen.pone.y.c
    COMMAND echo $(PROJECT_BINARY_DIR)
    COMMAND ./greg -o gen.pone.y.c ${PROJECT_SOURCE_DIR}/src/pone.y
    DEPENDS greg src/pone.y
    )
add_custom_command(
    OUTPUT src/gen.node.c
    COMMAND perl ${PROJECT_SOURCE_DIR}/build/node.pl ${PROJECT_SOURCE_DIR}/src/pvip.h src/gen.node.c 
    DEPENDS src/pvip.h build/node.pl
    )

add_subdirectory(3rd/onigmo-cmake)
