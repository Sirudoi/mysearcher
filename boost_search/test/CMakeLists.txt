cmake_minimum_required(VERSION 3.0)
project(testMuduo)

# 设置C++版本为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
add_definitions("-Wall -g")

set(MUDUO_PATH ../muduo/lib/)

find_library(LIBMUDUO NAMES libmymuduo.so PATHS ${MUDUO_PATH})

#设置源文件
set(SOURCES
    ${PROJ_ROOT}test.cc
)

# 添加可执行程序
add_executable(testMuduo ${SOURCES})

# 添加链接库
target_link_libraries(testMuduo pthread ${LIBMUDUO})
