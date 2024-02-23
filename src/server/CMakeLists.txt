aux_source_directory(. SRC_LIST)
aux_source_directory(./mysqlconnectionpool SRC_LIST1)
aux_source_directory(./model SRC_LIST2)
set(ALL_SRCS ${SRC_LIST} ${SRC_LIST1} ${SRC_LIST2})
message(STATUS "ALL_SRCS: ${ALL_SRCS}")
add_executable(ChatServer ${ALL_SRCS})

target_link_libraries(ChatServer mysqlcppconn muduo_net muduo_base pthread)
