set(SERVER_SOURCES
    main.cpp
    chatservice.cpp
    chatserver.cpp
    redisclient.cpp
    # 显式列出源码
    mysqlconnectionpool/mysqlconnectionpool.cpp
    redisconnectionpool/redisconnectionpool.cpp
    model/user.cpp
    model/usermodel.cpp
    model/friendmodel.cpp
    model/groupmodel.cpp
    model/offlinemessage.cpp
)

add_executable(ChatServer ${SERVER_SOURCES})

target_include_directories(ChatServer
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/server
        ${PROJECT_SOURCE_DIR}/include/server/model
        ${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool
        ${PROJECT_SOURCE_DIR}/include/server/redisconnectionpool
        ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(ChatServer
    PRIVATE
        mysqlcppconn
        muduo_net
        muduo_base
        pthread
        hiredis::hiredis
        redis++::redis++
)

target_compile_options(ChatServer PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>:-O2 -Wall>
)

set_target_properties(ChatServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)
