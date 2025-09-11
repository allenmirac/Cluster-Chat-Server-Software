cmake_minimum_required(VERSION 3.16)
project(Chat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# hiredis
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h
    PATHS /usr/include /usr/local/include
)
find_library(HIREDIS_LIBRARY hiredis
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
    message(STATUS "Found hiredis: ${HIREDIS_LIBRARY}")
    add_library(hiredis::hiredis UNKNOWN IMPORTED)
    set_target_properties(hiredis::hiredis PROPERTIES
        IMPORTED_LOCATION ${HIREDIS_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${HIREDIS_INCLUDE_DIR}
    )
else()
    message(FATAL_ERROR "hiredis not found")
endif()

# redis++
find_path(REDIS_PLUS_PLUS_INCLUDE_DIR sw/redis++/redis.h
    PATHS /usr/include /usr/local/include
)
find_library(REDIS_PLUS_PLUS_LIBRARY redis++
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)
if(REDIS_PLUS_PLUS_INCLUDE_DIR AND REDIS_PLUS_PLUS_LIBRARY)
    message(STATUS "Found redis++: ${REDIS_PLUS_PLUS_LIBRARY}")
    add_library(redis++::redis++ UNKNOWN IMPORTED)
    set_target_properties(redis++::redis++ PROPERTIES
        IMPORTED_LOCATION ${REDIS_PLUS_PLUS_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${REDIS_PLUS_PLUS_INCLUDE_DIR}
    )
else()
    message(FATAL_ERROR "redis-plus-plus not found")
endif()

# 全局 include (只放必要的，例如项目自己的 include)
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加子目录
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(test_pressure)
