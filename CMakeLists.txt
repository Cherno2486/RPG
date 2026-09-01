cmake_minimum_required(VERSION 3.20)
project(rpg_mazmorras CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/external)

# raylib se baja y compila sola la primera vez que se corre cmake (no hace
# falta instalarla aparte). Las builds siguientes son mucho mas rapidas.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(rpg_mazmorras
    src/main.cpp
    src/game/mathtypes.cpp
    src/game/character.cpp
    src/game/party.cpp
    src/game/dungeon.cpp
    src/game/dice.cpp
    src/game/effects.cpp
    src/game/combat_state.cpp
    src/game/enemy.cpp
    src/game/combat.cpp
    src/render/renderer.cpp
    src/render/input.cpp
    src/render/ui.cpp
    src/render/combat_ui.cpp
)

target_link_libraries(rpg_mazmorras PRIVATE raylib)

# En Windows, raylib necesita estas librerias del sistema para ventana/audio/graficos.
if (WIN32)
    target_link_libraries(rpg_mazmorras PRIVATE winmm gdi32)
endif()

# Copia assets/ al lado del ejecutable despues de cada build.
add_custom_command(TARGET rpg_mazmorras POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:rpg_mazmorras>/assets
)
