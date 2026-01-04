cmake_minimum_required(VERSION 3.16)
project(goldstein_cap1_opengl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 QUIET)

include(FetchContent)
if(NOT glfw3_FOUND)
  message(STATUS "GLFW no encontrado en el sistema. Se descargara con FetchContent")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
  )
  FetchContent_MakeAvailable(glfw)
endif()

find_package(OpenGL REQUIRED)

add_executable(goldstein_cap1 src/main.cpp)

if(TARGET glfw)
  target_link_libraries(goldstein_cap1 PRIVATE glfw OpenGL::GL)
elseif(TARGET glfw3::glfw)
  target_link_libraries(goldstein_cap1 PRIVATE glfw3::glfw OpenGL::GL)
else()
  message(FATAL_ERROR "No se encontro un objetivo de enlace de GLFW")
endif()

if(MSVC)
  target_compile_options(goldstein_cap1 PRIVATE /W4)
else()
  target_compile_options(goldstein_cap1 PRIVATE -Wall -Wextra -Wpedantic)
endif()
