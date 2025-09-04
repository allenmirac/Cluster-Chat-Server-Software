add_executable(testMuduo muduo_server.cpp)

find_library(MUDUO_BASE_LIB NAMES muduo_base)
find_library(MUDUO_NET_LIB NAMES muduo_net)
find_library(PTHREAD_LIB NAMES pthread)

if(NOT MUDUO_BASE_LIB)
    message(FATAL_ERROR "muduo_base not be found")
endif()
if(NOT MUDUO_NET_LIB)
    message(FATAL_ERROR "muduo_net not be found")
endif()
if(NOT PTHREAD_LIB)
    message(FATAL_ERROR "pthread not be found")
endif()

target_link_libraries(testMuduo
    ${MUDUO_BASE_LIB}
    ${MUDUO_NET_LIB}
    ${PTHREAD_LIB}
)

message(STATUS "Muduo libraries: ${MUDUO_BASE_LIB} ${MUDUO_NET_LIB} ${PTHREAD_LIB}")