cmake_minimum_required(VERSION 3.0)
project(boostSearch)

# 设置C++版本为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置最新的gcc和g++编译器路径 which gcc/g++
set(CMAKE_C_COMPILE /opt/rh/devtoolset-7/root/usr/bin/gcc)
set(CMAKE_CXX_COMPILE /opt/rh/devtoolset-7/root/usr/bin/g++)


# 获取当前源文件路径
get_filename_component(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# parser模块源文件
set(PARSER_SOURCES
    ${PROJ_ROOT}/parser.cc
)

# search模块源文件
set(SEARCH_SOURCES
    ${PROJ_ROOT}/search.cc
)

# 头文件模块
set(INCLUDES
    ${PROJ_ROOT}/index.hpp
    ${PROJ_ROOT}/parser.h
    ${PROJ_ROOT}/search.hpp
    ${PROJ_ROOT}/common/httplib.h
    ${PROJ_ROOT}/common/log.hpp
    ${PROJ_ROOT}/common/util.hpp
)

# 添加可执行程序
add_executable(boost_parser ${PARSER_SOURCES} ${HEADERS})
add_executable(boost_searcher ${SEARCH_SOURCES} ${HEADERS})

# 连接库
target_link_libraries(boost_parser boost_system boost_filesystem)
target_link_libraries(boost_searcher jsoncpp pthread)
