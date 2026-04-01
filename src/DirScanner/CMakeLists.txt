cmake_minimum_required(VERSION 3.15)
project(DirScanner VERSION 1.0.0 LANGUAGES CXX)

# C++17 required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

# ArgParser library (third_party)
add_library(ArgParser STATIC
    third_party/ArgParser.cpp
)
target_include_directories(ArgParser PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party
)

# DirScanner library
add_library(DirScanner STATIC
    src/dir_scan.cpp
)
target_include_directories(DirScanner PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Main executable
add_executable(dirscanner
    src/main.cpp
)
target_link_libraries(dirscanner
    PRIVATE
        DirScanner
        ArgParser
)

# Optional: Test executable
if(BUILD_TESTING)
    add_executable(dirscanner_test
        test/test.cpp
    )
    target_link_libraries(dirscanner_test PRIVATE DirScanner)
endif()
