cmake_minimum_required(VERSION 3.13)

project(lua C)

add_library(lua STATIC
    lapi.c
    lcode.c
    lctype.c   # ← これを追加
    ldebug.c
    ldo.c
    ldump.c
    lfunc.c
    lgc.c
    llex.c
    lmem.c
    lobject.c
    lopcodes.c
    lparser.c
    lstate.c
    lstring.c
    ltable.c
    ltm.c
    lundump.c
    lvm.c
    lzio.c
    lauxlib.c
    lbaselib.c
    lcorolib.c
    ldblib.c
    liolib.c
    lmathlib.c
    loslib.c
    lstrlib.c
    ltablib.c
    lutf8lib.c
    loadlib.c
    linit.c
)

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(lua PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)