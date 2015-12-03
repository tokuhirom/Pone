cmake_minimum_required(VERSION 2.8.7)
project(pone)

INCLUDE(CheckSymbolExists)

enable_testing()

include_directories(3rd/linenoise/ 3rd/onigmo/ src/ include)

list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(accept4 sys/socket.h HAVE_ACCEPT4)
check_symbol_exists(SOCK_CLOEXEC sys/socket.h HAVE_SOCK_CLOEXEC)
configure_file(include/pone_config.h.in include/pone_config.h)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()


SET(CMAKE_C_FLAGS   "-g")
# -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
SET(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS} ")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
message("-- type: ${CMAKE_BUILD_TYPE}")

set(PONE_COMPILE_FLAGS "-D_GNU_SOURCE -pthread -std=c99 -Wall -W -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-function -Wno-unknown-warning-option")

set(WERROR "0" CACHE BOOL "enable -Werror")
if (WERROR)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -Werror")
endif()

set(YY_DEBUG "0" CACHE BOOL "YY_DEBUG")
if (YY_DEBUG)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DYY_DEBUG")
endif()

set(DEBUG_THREAD "0" CACHE BOOL "enable thread debugging")
if (DEBUG_THREAD)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DDEBUG_THREAD")
endif()

set(DEBUG_MODULE "0" CACHE BOOL "enable module debugging")
if (DEBUG_MODULE)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DDEBUG_MODULE")
endif()

set(DEBUG_EXC "0" CACHE BOOL "enable EXC debugging")
if (DEBUG_EXC)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DDEBUG_EXC")
endif()

set(DEBUG_GC "0" CACHE BOOL "enable GC debugging")
if (DEBUG_GC)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DDEBUG_GC")
endif()

set(DEBUG_WORLD "0" CACHE BOOL "enable world tracing")
if (DEBUG_WORLD)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DDEBUG_WORLD")
endif()

set(GC_RD_LOCK_DEBUG "0" CACHE BOOL "enable GC rd lock tracing")
if (GC_RD_LOCK_DEBUG)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DGC_RD_LOCK_DEBUG")
endif()

set(NO_REUSE "0" CACHE BOOL "do not reuse free'd value")
if (NO_REUSE)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DNO_REUSE")
endif()

set(STRESS_GC "0" CACHE BOOL "stress gc")
if (STRESS_GC)
    SET(PONE_COMPILE_FLAGS "${PONE_COMPILE_FLAGS} -DSTRESS_GC")
endif()


SET(serial "0.0.1")
SET(soserial "1")

SET(SRCS
    src/compile.c
    src/path.c
    src/stat.c
    src/time.c src/module.c src/file.c src/runtime.c src/os.c src/arg.c src/errno.c src/opaque.c src/channel.c src/gc.c src/pair.c src/obj.c src/class.c src/alloc.c src/array.c src/bool.c src/builtin.c src/code.c src/map.c src/int.c src/nil.c src/num.c src/op.c src/pone.c src/scope.c src/str.c src/world.c src/universe.c src/iter.c src/exc.c src/range.c src/re.c src/thread.c src/signal.c
    src/tmpfile.c
    src/pvip_node.c src/pvip_string.c gen.pone.y.c src/gen.node.c
    )

add_executable(pone src/main.c 3rd/linenoise/linenoise.c)
target_link_libraries(pone libpone m onigmo dl pthread)
set_target_properties(pone
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin/"
    )
set_target_properties(pone
    PROPERTIES
    COMPILE_FLAGS "${PONE_COMPILE_FLAGS}"
    )
set_target_properties(pone
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    )

add_library(libpone SHARED ${SRCS})
target_link_libraries(libpone m onigmo dl pthread)
set_target_properties(libpone
    PROPERTIES
    COMPILE_FLAGS "${PONE_COMPILE_FLAGS}"
    )
set_target_properties(libpone
    PROPERTIES
    PREFIX ""
    )
set_target_properties(libpone
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    )

macro(pone_ext target sources output_name)
    add_library(${target} SHARED ${sources})
    target_link_libraries(${target} libpone)
    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "blib/")
    set_target_properties(${target} PROPERTIES PREFIX "")
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output_name})
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS "${PONE_COMPILE_FLAGS}")
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endmacro()

pone_ext(ext_socket ext/socket.c "socket")
pone_ext(ext_math ext/math.c "math")
pone_ext(ext_http_parser "3rd/picohttpparser/picohttpparser.c;ext/http_parser.c" "http/parser")
pone_ext(ext_json ext/json.c "json")

file(MAKE_DIRECTORY blib/http/)

ADD_LIBRARY(linenoise STATIC 3rd/linenoise/linenoise.c)
set_source_files_properties(3rd/linenoise/linenoise.c PROPERTIES COMPILE_FLAGS "-g -Wno-implicit-function-declaration")

add_test(NAME prove COMMAND prove t xt)

add_executable(greg 3rd/greg/greg.c 3rd/greg/tree.c 3rd/greg/compile.c)
set_target_properties(greg PROPERTIES COMPILE_FLAGS "-g -Wno-sign-compare")
set_target_properties(greg
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}
    )

add_custom_command(
    OUTPUT gen.pone.y.c
    COMMAND ${PROJECT_BINARY_DIR}/greg -o gen.pone.y.c ${PROJECT_SOURCE_DIR}/src/pone.y
    DEPENDS greg src/pone.y
    )
add_custom_command(
    OUTPUT src/gen.node.c
    COMMAND perl ${PROJECT_SOURCE_DIR}/build/node.pl ${PROJECT_SOURCE_DIR}/src/pvip.h src/gen.node.c 
    DEPENDS src/pvip.h build/node.pl
    )

add_subdirectory(3rd/onigmo-cmake)
