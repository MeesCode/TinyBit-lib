# TinyBit library CMakeLists.txt

add_library(tinybit_lib INTERFACE)

target_sources(tinybit_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/tinybit.c
    ${CMAKE_CURRENT_LIST_DIR}/lua_pool.c
    ${CMAKE_CURRENT_LIST_DIR}/cartridge.c
    ${CMAKE_CURRENT_LIST_DIR}/graphics.c
    ${CMAKE_CURRENT_LIST_DIR}/font.c
    ${CMAKE_CURRENT_LIST_DIR}/input.c
    ${CMAKE_CURRENT_LIST_DIR}/audio.c
    ${CMAKE_CURRENT_LIST_DIR}/memory.c
    ${CMAKE_CURRENT_LIST_DIR}/lua_functions.c
    ${CMAKE_CURRENT_LIST_DIR}/pngle/pngle.c
    ${CMAKE_CURRENT_LIST_DIR}/pngle/miniz.c
    ${CMAKE_CURRENT_LIST_DIR}/helpers.c
    ${CMAKE_CURRENT_LIST_DIR}/ABC-parser/abc_parser.c
)

# Add Lua sources
file(GLOB LUA_SOURCES ${CMAKE_CURRENT_LIST_DIR}/lua/*.c)
target_sources(tinybit_lib INTERFACE ${LUA_SOURCES})

target_include_directories(tinybit_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/lua
    ${CMAKE_CURRENT_LIST_DIR}/pngle
    ${CMAKE_CURRENT_LIST_DIR}/ABC-parser
)
