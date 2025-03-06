cmake_minimum_required (VERSION 3.1)
project(LearnTinyrenderer)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(src)
