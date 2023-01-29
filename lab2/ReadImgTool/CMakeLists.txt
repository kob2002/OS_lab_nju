cmake_minimum_required(VERSION 3.21)
project(ReadImgTool)

set(CMAKE_CXX_STANDARD 14)

add_executable(ReadImgTool main.cpp Tool.cpp Tool.h ubuntuVersion.cpp)
