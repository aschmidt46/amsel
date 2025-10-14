cmake_minimum_required(VERSION 3.20)

# C++ Laufzeitbibliothek statisch verlinken auf MSVC
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

project(antons-nes-emu)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include_directories(PUBLIC include)

find_package(glfw3 REQUIRED)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)


include_directories(PUBLIC .)
include_directories(PUBLIC imgui)
include_directories(PUBLIC imgui/backends)



add_library(cpu source/6502.cpp source/6502.h)
add_library(ppu source/ppu.cpp source/ppu.h)
add_library(cputest source/cputest.cpp source/cputest.h)
add_library(nesfile source/nes_file.cpp source/nes_file.h)
add_library(mapper source/mapper.cpp source/mapper.h)
add_library(screen source/screen.cpp source/screen.h)
add_library(controller source/controller.cpp source/controller.h)
add_library(nes source/nes.cpp source/nes.h)
add_library(palette source/palette.cpp source/palette.h)
add_library(gui source/gui.cpp source/gui.h)

add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp
  imgui/backends/imgui_impl_glfw.cpp
  imgui/backends/imgui_impl_opengl3.cpp
  imgui/misc/cpp/imgui_stdlib.cpp
)


add_executable(nes-emu source/main.cpp)
target_link_libraries(nes-emu cputest nes palette nesfile cpu ppu mapper controller screen gui)
target_link_libraries(nes-emu OpenGL::GL)
target_link_libraries(nes-emu imgui)
target_link_libraries(nes-emu glfw)
target_link_libraries(nes-emu GLEW::GLEW)


file(COPY "lib/glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "resources/palette.pal" DESTINATION ${CMAKE_BINARY_DIR})
