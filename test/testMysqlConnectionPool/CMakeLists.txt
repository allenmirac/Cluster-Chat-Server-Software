# 添加运行的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(/home/mirac/Desktop/Github/Cluster-Chat-Server-Software/src/server/mysqlconnectionpool SRC_LIST1)
message(STATUS "SRC_LIST1: ${SRC_LIST1}")
set(ALL_SRCS ${SRC_LIST} ${SRC_LIST1})

# 可执行文件
add_executable(testMysqlConnectionPool ${ALL_SRCS})
message(STATUS "ALL_SRCS: ${ALL_SRCS}")

# 包含的头文件
include_directories(${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool)
message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")

# 需要链接的库
find_library(MYSQLCPPCONN_LIB mysqlcppconn muduo_net muduo_base pthread)
if(MYSQLCPPCONN_LIB)
    target_link_libraries(testMysqlConnectionPool ${MYSQLCPPCONN_LIB})
else()
    message(FATAL_ERROR "mysqlcppconn library not found")
endif()

# target_link_libraries(mysqlcppconn muduo_base)