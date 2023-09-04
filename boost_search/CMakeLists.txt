cmake_minimum_required(VERSION 3.0)
project(boostSearch)

# 设置C++版本为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
add_definitions("-Wall -g")

# 设置最新的gcc和g++编译器路径 which gcc/g++
set(CMAKE_C_COMPILE /opt/rh/devtoolset-7/root/usr/bin/gcc)
set(CMAKE_CXX_COMPILE /opt/rh/devtoolset-7/root/usr/bin/g++)


# 获取当前源文件路径
get_filename_component(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# parser模块源文件
set(PARSER_SOURCES
    ${PROJ_ROOT}/src/parser.cc
    ${PROJ_ROOT}/src/threadpool.cc
    ${PROJ_ROOT}/src/index.cc
    ${PROJ_ROOT}/src/util.cc
)

# search模块源文件
set(SEARCH_SOURCES
    ${PROJ_ROOT}/src/search.cc
    ${PROJ_ROOT}/src/index.cc
    ${PROJ_ROOT}/src/threadpool.cc
    ${PROJ_ROOT}/src/util.cc

)

# parser头文件
set(PARSER_HEADERS
    ${PROJ_ROOT}/include/index.h
    ${PROJ_ROOT}/include/parser.h
    ${PROJ_ROOT}/include/threadpool.h
    ${PROJ_ROOT}/include/log.hpp
    ${PROJ_ROOT}/include/util.h
)

set(SEARCHER_HEADERS
    ${PROJ_ROOT}/include/index.h
    ${PROJ_ROOT}/include/threadpool.h
    ${PROJ_ROOT}/include/search.hpp
    ${PROJ_ROOT}/include/httplib.h
    ${PROJ_ROOT}/include/log.hpp
    ${PROJ_ROOT}/include/util.h
)

# 添加可执行程序
add_executable(boost_parser ${PARSER_SOURCES} ${PARSER_HEADERS})
add_executable(boost_searcher ${SEARCH_SOURCES} ${SEARCHER_HEADERS})

# 连接库
target_link_libraries(boost_parser boost_system boost_filesystem pthread)
target_link_libraries(boost_searcher jsoncpp pthread)
