set(CLIENT_SOURCES
    main.cpp
    ../server/model/user.cpp   # 复用 server 中的 user.cpp
)

add_executable(ChatClient ${CLIENT_SOURCES})

target_include_directories(ChatClient
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/client
        ${PROJECT_SOURCE_DIR}/include/server/model
        ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(ChatClient
    PRIVATE
        pthread
)

set_target_properties(ChatClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)
