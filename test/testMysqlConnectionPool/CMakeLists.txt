# 添加运行的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/server/mysqlconnectionpool SRC_LIST1)
set(ALL_SRCS ${SRC_LIST} ${SRC_LIST1})
# message(STATUS "ALL_SRCS: ${ALL_SRCS}")
# 包含的头文件
# include_directories(${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool)
# message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")

# 可执行文件
add_executable(testMysqlConnectionPool ${ALL_SRCS})
# message(STATUS "ALL_SRCS: ${ALL_SRCS}")

# 需要链接的库
# 查找mysqlcppconn库
find_library(MYSQLCPPCONN_LIB NAMES mysqlcppconn)
# 查找muduo_base库
find_library(MUDUO_BASE_LIB NAMES muduo_base)

# 检查是否所有库都已找到
if(NOT MYSQLCPPCONN_LIB)
    message(FATAL_ERROR "mysqlcppconn library not found")
endif()

if(NOT MUDUO_BASE_LIB)
    message(FATAL_ERROR "muduo_base library not found")
endif()

target_link_libraries(testMysqlConnectionPool 
    ${MYSQLCPPCONN_LIB}
    ${MUDUO_BASE_LIB}
    pthread
)
message(STATUS "target_link_libraries: ${MYSQLCPPCONN_LIB} ${MUDUO_BASE_LIB}")