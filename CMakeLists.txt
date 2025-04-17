cmake_minimum_required(VERSION 3.16)
project(FileSelector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(fmt REQUIRED)

# Component control options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" ON)
option(BUILD_EXECUTABLE "Build the executable" ON)

# Library sources (exclude main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)

# Conditionally build libraries
if(BUILD_STATIC_LIBS)
    add_library(FileSelectorStatic STATIC ${LIB_SOURCES})
    target_include_directories(FileSelectorStatic PUBLIC src)
    target_link_libraries(FileSelectorStatic PUBLIC fmt::fmt)
    set_target_properties(FileSelectorStatic PROPERTIES OUTPUT_NAME FileSelector)
endif()

if(BUILD_SHARED_LIBS)
    add_library(FileSelectorShared SHARED ${LIB_SOURCES})
    target_include_directories(FileSelectorShared PUBLIC src)
    target_link_libraries(FileSelectorShared PUBLIC fmt::fmt)
    set_target_properties(FileSelectorShared PROPERTIES OUTPUT_NAME FileSelector)
    endif()

# Conditionally build executable
if(BUILD_EXECUTABLE)
    add_executable(FileSelectorApp main.cpp ${LIB_SOURCES})
    target_include_directories(FileSelectorApp PRIVATE src)
    target_link_libraries(FileSelectorApp PRIVATE fmt::fmt)
endif()

install(FILES src/FileSelector.hpp DESTINATION include)
