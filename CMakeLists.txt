# originally it has bmf2mof.c including bmfparse.c, and bmfparse.c in turn includes bmfdec.c
# so bmf2mof.o is the executable object


cmake_minimum_required(VERSION 3.23)
project(bmfdec-refresh VERSION 0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND LIB_SRCS parser.c decompress.c printer.c)
add_library(bmfparser SHARED ${LIB_SRCS})
target_include_directories(bmfparser PUBLIC include)

add_executable(bmf2mof cli.c)
target_link_libraries(bmf2mof PRIVATE bmfparser)