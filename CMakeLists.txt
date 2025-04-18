cmake_minimum_required(VERSION 3.16)
project(FileSelector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(fmt CONFIG REQUIRED)

# Component control options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" ON)
option(BUILD_EXECUTABLE "Build the executable" ON)

# Set output directories for all targets by platform and configuration
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(UNIX)
    set(PLATFORM_NAME "linux")
else()
    set(PLATFORM_NAME "other")
endif()

foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(OUTDIR "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${OUTPUTCONFIG}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${OUTDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${OUTDIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${OUTDIR})
endforeach()

# For single-config generators (e.g. Makefiles)
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(OUTDIR "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/$<CONFIG>")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTDIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTDIR})
endif()

# Library sources (exclude main.cpp)
file(GLOB_RECURSE ALL_SOURCES src/*.cpp)
list(FILTER ALL_SOURCES EXCLUDE REGEX "main.cpp$")
set(LIB_SOURCES ${ALL_SOURCES})

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
