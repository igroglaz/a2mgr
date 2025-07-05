cmake_minimum_required(VERSION 3.10)

project(a2mgr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

link_libraries(ws2_32)

add_library(a2mgr SHARED a2mgr.def
    a2mgr.cpp
    add_info.cpp
    administrat_fix.cpp
    chat.cpp
    chat_history.cpp
    class_extend.cpp
    class_mapobject.cpp
    client_cmd.cpp
    config.cpp
    crash.cpp
    CRC_32.cpp
    debug.cpp
    File.cpp
    grid.cpp
    gui_bugfix.cpp
    gui_replace.cpp
    hotkey.cpp
    Image.cpp
    itempng.cpp
    landscape_renderer.cpp
    language.cpp
    map_location.cpp
    menu.cpp
    misc.cpp
    net_shared.cpp
    network_crypt.cpp
    network_hat_enter.cpp
    network_protocol.cpp
    network_protocol_ext.cpp
    pack_scrolling.cpp
    resource_control.cpp
    resource_files_replace.cpp
    scanrange.cpp
    screenshot.cpp
    scrolling.cpp
    serialize.cpp
    utils.cpp
    video_init.cpp
    zxmgr.cpp
    zxmgr_gui.cpp
    lib/happyhttp.cpp
    lib/hat2.cpp
    lib/lodepng.cpp
    lib/packet.cpp
    lib/ScanrangeCalc.cpp
    lib/serialize.cpp
    lib/sha1.cpp
    lib/socket.cpp
    lib/utils.cpp
)

target_include_directories(a2mgr PUBLIC sdl/include)

# Static link.
set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
target_link_directories(a2mgr PUBLIC sdl/lib)
target_link_libraries(a2mgr SDL SDL_image)
target_compile_options(a2mgr PUBLIC /MT)
target_link_options(a2mgr PUBLIC /NODEFAULTLIB:MSVCRT)

# Postbuild commands.
add_custom_command(TARGET a2mgr
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND copy build\\Release\\a2mgr.dll postbuild\\a2mgr.dll
)

add_custom_command(TARGET a2mgr
    POST_BUILD
    COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/postbuild/a2mgr.bat
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/postbuild
)
