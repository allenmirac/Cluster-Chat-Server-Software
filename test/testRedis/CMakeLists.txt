set(TESTMYSQL_COURCE 
    testRedis.cpp
    ../../src/server/redisconnectionpool/redisconnectionpool.cpp
    ../../src/server/redisclient.cpp
)

add_executable(testRedis ${TESTMYSQL_COURCE})

target_include_directories(testRedis PRIVATE
    ${PROJECT_SOURCE_DIR}/include/server/redisconnectionpool
    ${PROJECT_SOURCE_DIR}/include/server/
)

target_link_libraries(testRedis 
    PRIVATE
        muduo_net
        muduo_base
        hiredis::hiredis
        redis++::redis++
)

target_compile_options(testRedis PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>: -O2 -Wall>
)

set_target_properties(testRedis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)