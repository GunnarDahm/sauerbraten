cmake_minimum_required(VERSION 3.20)
project(sauerbraten C CXX)

set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src)

if(NOT EXISTS ${DEPS}/SDL2/CMakeLists.txt)
    message(FATAL_ERROR "Third-party sources missing: run android/fetch-deps.sh first")
endif()

# SDL2 itself is a shared library, loaded by SDLActivity
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${DEPS}/SDL2 SDL2)

# the SDL add-on libraries are linked statically into libmain.so
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_PNG_SAVE OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JPG_SAVE OFF CACHE BOOL "" FORCE)
foreach(fmt AVIF JXL TIF WEBP SVG XCF XPM XV LBM PCX PNM QOI GIF)
    set(SDL2IMAGE_${fmt} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(${DEPS}/SDL2_image SDL2_image)

set(SDL2MIXER_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_CMD OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_VORBIS "STB" CACHE STRING "" FORCE)
foreach(fmt FLAC GME MOD MP3 MIDI OPUS WAVPACK)
    set(SDL2MIXER_${fmt} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(${DEPS}/SDL2_mixer SDL2_mixer)

# gl4es: runs the engine's desktop OpenGL code on top of OpenGL ES 2.
# The context comes from SDL, so no EGL/X11 and no dynamic loader inside gl4es.
set(NOX11 ON CACHE BOOL "" FORCE)
set(NOEGL ON CACHE BOOL "" FORCE)
set(NO_LOADER ON CACHE BOOL "" FORCE)
set(NO_INIT_CONSTRUCTOR ON CACHE BOOL "" FORCE)
set(STATICLIB ON CACHE BOOL "" FORCE)
set(USE_ANDROID_LOG ON CACHE BOOL "" FORCE)
set(DEFAULT_ES 2 CACHE STRING "" FORCE)
add_subdirectory(${DEPS}/gl4es gl4es)

# enet
file(GLOB ENET_SOURCES ${SRC}/enet/*.c)
list(FILTER ENET_SOURCES EXCLUDE REGEX ".*/win32\\.c$")
add_library(enet STATIC ${ENET_SOURCES})
target_include_directories(enet PUBLIC ${SRC}/enet/include)
target_compile_definitions(enet PRIVATE HAS_POLL=1 HAS_FCNTL=1 HAS_INET_PTON=1 HAS_INET_NTOP=1
    HAS_MSGHDR_FLAGS=1 HAS_SOCKLEN_T=1 HAS_GETADDRINFO=1 HAS_GETNAMEINFO=1)

# the game, built as libmain.so (SDLActivity calls its SDL_main)
set(GAME_SOURCES
    shared/crypto.cpp shared/geom.cpp shared/stream.cpp shared/tools.cpp shared/zip.cpp
    engine/3dgui.cpp engine/bih.cpp engine/blend.cpp engine/blob.cpp engine/client.cpp
    engine/command.cpp engine/console.cpp engine/cubeloader.cpp engine/decal.cpp
    engine/dynlight.cpp engine/gamepad.cpp engine/glare.cpp engine/grass.cpp
    engine/lightmap.cpp engine/main.cpp engine/material.cpp engine/menus.cpp
    engine/movie.cpp engine/normal.cpp engine/octa.cpp engine/octaedit.cpp
    engine/octarender.cpp engine/physics.cpp engine/pvs.cpp engine/rendergl.cpp
    engine/rendermodel.cpp engine/renderparticles.cpp engine/rendersky.cpp
    engine/rendertext.cpp engine/renderva.cpp engine/server.cpp engine/serverbrowser.cpp
    engine/shader.cpp engine/shadowmap.cpp engine/sound.cpp engine/texture.cpp
    engine/water.cpp engine/world.cpp engine/worldio.cpp
    fpsgame/ai.cpp fpsgame/client.cpp fpsgame/entities.cpp fpsgame/fps.cpp
    fpsgame/monster.cpp fpsgame/movable.cpp fpsgame/render.cpp fpsgame/scoreboard.cpp
    fpsgame/server.cpp fpsgame/waypoint.cpp fpsgame/weapon.cpp)
list(TRANSFORM GAME_SOURCES PREPEND ${SRC}/)

add_library(main SHARED ${GAME_SOURCES})
target_include_directories(main PRIVATE ${SRC}/shared ${SRC}/engine ${SRC}/fpsgame ${DEPS}/gl4es/include)
target_compile_definitions(main PRIVATE USE_GLES)
# -fsigned-char matters: ARM defaults to unsigned char, the engine assumes signed
target_compile_options(main PRIVATE -O2 -ffast-math -fsigned-char -fno-exceptions -fno-rtti
    -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations)
target_link_libraries(main PRIVATE SDL2::SDL2 SDL2_image::SDL2_image-static SDL2_mixer::SDL2_mixer-static
    GL enet z log android m dl)
