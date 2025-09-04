cmake_minimum_required(VERSION 3.0)
project(chat)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug or Release)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/server/model
    ${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool
    ${PROJECT_SOURCE_DIR}/thirdparty
)

# 将子目录添加到当前项目中，并且通过子目录的 CMakeLists.txt 来构建项目
# 当前目录就是父目录，并且上面设置的变量可以给子目录中的 CMakeLists.txt 使用
add_subdirectory(src)
add_subdirectory(test)