cmake_minimum_required(VERSION 2.8.7)
project(pone)

enable_testing()

include_directories(3rd/linenoise/ 3rd/rockre/include 3rd/pvip/src/ src/)

# -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
SET(CMAKE_C_FLAGS_DEBUG   "-fPIC -D_POSIX_C_SOURCE=200809L -pthread -std=c99 -Wall -W -Wextra -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function -g")
SET(CMAKE_C_FLAGS_RELEASE "-fPIC -D_POSIX_C_SOURCE=200809L -pthread -std=c99 -Wall -W -Wextra -Wno-unused-but-set-variable -Wno-unused-function -DNDEBUG")
SET(CMAKE_BUILD_TYPE Debug)

SET(serial "0.0.1")
SET(soserial "1")

SET(SRCS
    src/gc.c src/pair.c src/obj.c src/class.c src/alloc.c src/array.c src/bool.c src/builtin.c src/code.c src/hash.c src/int.c src/nil.c src/num.c src/op.c src/pone.c src/scope.c src/str.c src/world.c src/universe.c src/iter.c src/exc.c src/range.c src/mu.c src/any.c src/cool.c src/socket.c src/re.c src/thread.c src/signal.c
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

