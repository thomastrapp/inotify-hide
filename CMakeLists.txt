cmake_minimum_required (VERSION 2.6)
project(inotify-hide)

#set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("${PROJECT_SOURCE_DIR}")

add_executable(inotify-hide main.c inoh/print.c inoh/file.c inoh/ino-hide.c)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-std=c99" "-pedantic" "-Wall" "-Wshadow" "-Wpointer-arith" "-Wcast-qual" "-Wstrict-prototypes")
endif()

