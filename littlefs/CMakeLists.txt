# LittleFS library for Raspberry Pi Pico
cmake_minimum_required(VERSION 3.13)

# Create littlefs library
add_library(littlefs STATIC
    lfs.c
    lfs_util.c
)

target_include_directories(littlefs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with pico stdlib
target_link_libraries(littlefs
    pico_stdlib
)

# Compiler options for embedded systems
target_compile_definitions(littlefs PRIVATE
    LFS_NO_DEBUG
    LFS_NO_WARN
    LFS_NO_ERROR
)