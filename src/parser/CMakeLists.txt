cmake_minimum_required(VERSION 2.8)
project(parser)
set(CMAKE_C_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH ./bin/)
set(LIBRARY_OUTPUT_PATH ./lib/)

add_library(parser parser.c)
