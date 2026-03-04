cmake_minimum_required(VERSION 3.25)
project(FileCopierDemo VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(fcopy FileCopier.cpp demo.cpp)
