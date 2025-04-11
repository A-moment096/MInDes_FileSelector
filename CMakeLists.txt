cmake_minimum_required(VERSION 3.10.0)
project(File_Selector VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
find_package(fmt REQUIRED)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
find_package(fmt PATHS C:/Developing/Libraries/fmt-11.1.4/build REQUIRED)
endif()

add_executable(File_Selector main.cpp)
target_include_directories(File_Selector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(File_Selector PRIVATE fmt::fmt)