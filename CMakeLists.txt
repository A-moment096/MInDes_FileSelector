# cmake_minimum_required(VERSION 3.10.0)
# project(File_Selector VERSION 0.1.0 LANGUAGES C CXX)

# set(CMAKE_CXX_STANDARD 17)

# if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
# find_package(fmt REQUIRED)
# elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
# find_package(fmt CONFIG REQUIRED)
# endif()

# add_executable(File_Selector main.cpp)
# target_include_directories(File_Selector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(File_Selector PRIVATE fmt::fmt)

cmake_minimum_required(VERSION 3.16)
project(FileSelectorApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add fmt library (make sure it's installed or use FetchContent)
find_package(fmt REQUIRED)

# Add source directory
include_directories(src)

# Collect all source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Define executable
add_executable(FileSelectorApp main.cpp ${SOURCES})

# Link libraries
target_link_libraries(FileSelectorApp PRIVATE fmt::fmt)
