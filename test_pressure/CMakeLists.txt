set(TESTPRESSURE_SOURCE
    test_pressure.cpp
)

add_executable(testPressure ${TESTPRESSURE_SOURCE})

# target_include_directories(testPressure PRIVATE
# )

target_link_libraries(testPressure
    PRIVATE
        pthread
)

target_compile_options(testPressure PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>: -O2 -Wall>
)

set_target_properties(testPressure PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)