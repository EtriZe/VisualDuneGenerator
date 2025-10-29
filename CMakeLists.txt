cmake_minimum_required(VERSION 3.20)
project(dune_viewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === SDL2 ===
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# === OpenGL + GLEW ===
find_package(OpenGL REQUIRED)
find_path(GLEW_INCLUDE_DIR GL/glew.h)
find_library(GLEW_LIBRARY NAMES GLEW glew32 glew)
if (NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARY)
    message(FATAL_ERROR "GLEW not found. Install libglew-dev or equivalent.")
endif()
include_directories(${GLEW_INCLUDE_DIR})

# === ImGui (local) ===
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

# === Notre exécutable ===
add_executable(dune_viewer
    dune_viewer.cpp
    ${IMGUI_SOURCES}
)

target_link_libraries(dune_viewer
    ${SDL2_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${GLEW_LIBRARY}
)
