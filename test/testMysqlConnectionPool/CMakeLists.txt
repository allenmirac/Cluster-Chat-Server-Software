# 添加运行的源文件
aux_source_directory(. SRC_LIST)
# 可执行文件
add_executable(testMysqlConnectionPool ${SRC_LIST})
# 需要链接的库
find_library(MYSQLCPPCONN_LIB mysqlcppconn muduo_base)
if(MYSQLCPPCONN_LIB)
    target_link_libraries(testMysqlConnectionPool ${MYSQLCPPCONN_LIB})
else()
    message(FATAL_ERROR "mysqlcppconn library not found")
endif()

# target_link_libraries(mysqlcppconn muduo_base)