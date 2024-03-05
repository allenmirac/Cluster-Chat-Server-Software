aux_source_directory(. SRC_LIST)
set(SRC_LIST1 ../server/model/user.cpp)
# aux_source_directory(../server/model SRC_LIST1)
message(STATUS "ALL_SRCSCLIENT: ${SRC_LIST} ${SRC_LIST1}")

add_executable(ChatClient ${SRC_LIST} ${SRC_LIST1})

target_link_libraries(ChatClient pthread)