set(TESTJSON_SOURCES
    testJson.cpp
)
add_executable(testJson ${TESTJSON_SOURCES})

target_include_directories(testJson
    PRIVATE
        ${PROJECT_SOURCE_DIR}/thirdparty
)

target_compile_options(testJson PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>: -O2 -Wall>
)

set_target_properties(testJson PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)