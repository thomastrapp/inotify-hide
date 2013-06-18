cmake_minimum_required(VERSION 2.6)
project(inotify-hide)

#set(CMAKE_VERBOSE_MAKEFILE on)

option(SCAN_BUILD "Static analysis with clang's ccc-analyzer" OFF)

if(SCAN_BUILD)
  SET(CMAKE_C_FLAGS "-Weverything -g")
  SET(CMAKE_C_COMPILER "/usr/share/clang/scan-build/ccc-analyzer")
  SET(CMAKE_COMPILER_IS_GNUCXX OFF)
endif(SCAN_BUILD)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-std=c99" "-pedantic" "-Wall" "-Wextra" "-Wshadow" "-Wpointer-arith" "-Wcast-qual" "-Wstrict-prototypes" "-Wmissing-prototypes" "-Wold-style-definition" "-Wpadded" "-Wcast-align")
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories("${PROJECT_SOURCE_DIR}")

add_executable(inotify-hide main.c inoh/print.c inoh/file.c inoh/ino-hide.c inoh/ino-hide-worker.c inoh/signal-handlers.c)

# enable posix 2008 (probably only for strdup; if posix 2008 
# is unavailable, -D_XOPEN_SOURCE=500 might suffice
add_definitions(-D_XOPEN_SOURCE=700)

