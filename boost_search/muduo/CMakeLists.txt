cmake_minimum_required(VERSION 3.0)
project(mymuduo)

# 编译成so动态库 设置动态库的路径 放置项目根目录的lib文件夹下面
# PROJECT_SOURCE_DIR是CMakeLists.txt同级目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置头文件所在路径
include_directories(
    ${PROJECT_SOURCE_DIR}/muduo_include
    ${PROJECT_SOURCE_DIR}/net/net_include
    ${PROJECT_SOURCE_DIR}/include/log.hpp
)

# 包含muduo_src目录下所有文件
aux_source_directory(muduo_src _SOURCES)

# 包含net/net_src目录下所有文件
aux_source_directory(net/net_src _SOURCES)

# 设置携带调试信息 C++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 编译动态库
add_library(mymuduo SHARED ${_SOURCES})