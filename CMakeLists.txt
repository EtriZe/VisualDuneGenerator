cmake_minimum_required(VERSION 3.16)
project(dune_viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

file(GLOB SOURCES
  dune_viewer.cpp
  imgui/*.cpp
  imgui/backends/imgui_impl_sdl2.cpp
  imgui/backends/imgui_impl_opengl2.cpp
)

add_executable(dune_viewer ${SOURCES})

target_include_directories(dune_viewer PRIVATE
  ${CMAKE_SOURCE_DIR}/imgui
  ${CMAKE_SOURCE_DIR}/imgui/backends
)

find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)

target_compile_definitions(dune_viewer PRIVATE
  GLEW_STATIC
  SDL_MAIN_HANDLED
)

target_link_libraries(dune_viewer PRIVATE
  SDL2::SDL2
  SDL2::SDL2main
  GLEW::GLEW
  opengl32
)