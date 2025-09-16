cmake_minimum_required(VERSION 3.31)
project(tools)

set(CMAKE_C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(bin2c bin2c.c)
