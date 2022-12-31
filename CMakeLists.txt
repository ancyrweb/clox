cmake_minimum_required(VERSION 3.0.0)
project(clox VERSION 0.1.0)

add_executable(clox compiler.c scanner.c vm.c value.c debug.c memory.c chunk.c main.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_C_STANDARD 99)
