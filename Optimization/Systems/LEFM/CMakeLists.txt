# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(LEFMProject VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/BufferData.cpp
    src/LEFMData.cpp
    src/main.cpp
    src/OtherFile1.cpp
    src/OtherFile2.cpp
)

# Add executable
add_executable(LEFMApp ${SOURCES})

# Optional: compile flags
target_compile_options(LEFMApp PRIVATE -Wall -O2)

# Optional: link libraries
# target_link_libraries(LEFMApp PRIVATE <library_name>)
