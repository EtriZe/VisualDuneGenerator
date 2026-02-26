cmake_minimum_required(VERSION 3.16)
project(dune_viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

# --- Sources ---
file(GLOB IMGUI_SOURCES
  ${CMAKE_SOURCE_DIR}/imgui/*.cpp
  ${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.cpp
  ${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_opengl2.cpp
)

set(APP_SOURCES
  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/App.cpp
  ${CMAKE_SOURCE_DIR}/src/Noise.cpp
  ${CMAKE_SOURCE_DIR}/src/TerrainGenerator.cpp
  ${CMAKE_SOURCE_DIR}/src/HeightmapIO.cpp
  ${CMAKE_SOURCE_DIR}/src/Renderer.cpp
  ${CMAKE_SOURCE_DIR}/src/ConfigKV.cpp
  ${CMAKE_SOURCE_DIR}/src/UI.cpp
)

add_executable(dune_viewer
  ${APP_SOURCES}
  ${IMGUI_SOURCES}
)

target_include_directories(dune_viewer PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/imgui
  ${CMAKE_SOURCE_DIR}/imgui/backends
)

# SDL2 flags via sdl2-config (comme tu faisais)
execute_process(COMMAND sdl2-config --cflags OUTPUT_VARIABLE SDL2_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sdl2-config --libs   OUTPUT_VARIABLE SDL2_LIBS   OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(SDL2_CFLAGS)
separate_arguments(SDL2_LIBS)

target_compile_options(dune_viewer PRIVATE ${SDL2_CFLAGS})
target_link_libraries(dune_viewer PRIVATE ${SDL2_LIBS})

target_compile_definitions(dune_viewer PRIVATE
  SDL_MAIN_HANDLED
)

# Si tu utilises GLEW en statique, garde GLEW_STATIC, sinon enlève cette ligne
# target_compile_definitions(dune_viewer PRIVATE GLEW_STATIC)

target_link_libraries(dune_viewer PRIVATE
  GLEW::GLEW
  OpenGL::GL
)