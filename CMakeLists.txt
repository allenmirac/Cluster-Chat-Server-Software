cmake_minimum_required(VERSION 3.0)
project(chat)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/testMuduo/src SRC)
# MESSAGE(STATUS "The value of SRC is ${SRC}")
# 表示可执行文件 server ，由 SRC 变量所定义的源文件编译而来
# add_executable(server ${SRC})
# 表示 server 这个目标程序，需要连接 muduo... 等库文件
# target_link_libraries(server muduo_net muduo_base pthread)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool)
set(HEAD_PATH_MYSQL_POOL ${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool)
message(STATUS "Include directories: ${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool")
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)

# 将子目录添加到当前项目中，并且通过子目录的 CMakeLists.txt 来构建项目
# 当前目录就是父目录，并且上面设置的变量可以给子目录中的 CMakeLists.txt 使用
# add_subdirectory(src)
add_subdirectory(test)