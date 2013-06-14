cmake_minimum_required(VERSION 2.6)
project(inotify-hide)

#set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("${PROJECT_SOURCE_DIR}")

add_executable(inotify-hide main.c inoh/print.c inoh/file.c inoh/ino-hide.c)

# enable posix 2008 (probably only for strdup; if posix 2008 
# is unavailable, -D_XOPEN_SOURCE=500 might suffice
add_definitions(-D_XOPEN_SOURCE=700)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-std=c99" "-pedantic" "-Wall" "-Wshadow" "-Wpointer-arith" "-Wcast-qual" "-Wstrict-prototypes")
endif()

