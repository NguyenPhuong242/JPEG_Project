cmake_minimum_required(VERSION 3.22)
project(JPEG_Compressor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
# Remove deleted rle.cpp from SOURCES
list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/src/entropy/rle.cpp")

# Create a library from your core source files (e.g., dct.cpp, quant.cpp)
add_library(jpeg_core STATIC
    ${SOURCES}
    src/core/cHuffman.cpp
)

# Make the headers in the 'include' directory available to targets that link against jpeg_core
target_include_directories(jpeg_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Create the main executable for the project
# add_executable(jpeg_compressor src/main.cpp)
# target_link_libraries(jpeg_compressor PRIVATE jpeg_core)

enable_testing()
add_subdirectory(tests)
