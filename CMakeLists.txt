cmake_minimum_required (VERSION 3.1)
project(tinyrenderer)

set(CMAKE_CXX_STANDARD 17)

# 输出 C++ 标准版本
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")

find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB SOURCES src/*.h src/*.cpp)

add_executable(tinyrenderer ./src/tgaimage.cpp ./src/main.cpp ./src/renderer.cpp)
