set(TESTMYSQL_COURCE 
    testMysqlConnectionPool.cpp
    ../../src/server/mysqlconnectionpool/mysqlconnectionpool.cpp
)

add_executable(testMysqlConnectionPool ${TESTMYSQL_COURCE})

target_include_directories(testMysqlConnectionPool PRIVATE
    ${PROJECT_SOURCE_DIR}/include/server/mysqlconnectionpool
)

target_link_libraries(testMysqlConnectionPool 
    PRIVATE
        muduo_base
        muduo_net
        mysqlcppconn
)

target_compile_options(testMysqlConnectionPool PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>: -O2 -Wall>
)

set_target_properties(testMysqlConnectionPool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)