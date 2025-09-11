set(TESTMUDUO_SOURCES
    testMuduo.cpp
)

add_executable(testMuduo ${TESTMUDUO_SOURCES})

target_link_libraries(testMuduo
    PRIVATE
        muduo_net
        muduo_base
        pthread
)

target_compile_options(testMuduo PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>: -O2 -Wall>
)

set_target_properties(testMuduo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)