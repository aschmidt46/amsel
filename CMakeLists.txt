cmake_minimum_required(VERSION 3.20)

# C++ Laufzeitbibliothek statisch verlinken auf MSVC
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

project(antons-nes-emu)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
add_compile_definitions(NES_ON_WINDOWS)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)
add_link_options(-mwindows)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include_directories(PUBLIC include)

find_package(glfw3 REQUIRED)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)

set(RTAUDIO_BUILD_STATIC_LIBS TRUE)
add_subdirectory(deps/rtaudio)

include_directories(PUBLIC .)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami)


add_library(imgui STATIC
  deps/imgui/imgui.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/imgui_widgets.cpp
  deps/imgui/imgui_tables.cpp
  deps/imgui/backends/imgui_impl_glfw.cpp
  deps/imgui/backends/imgui_impl_opengl3.cpp
  deps/imgui/misc/cpp/imgui_stdlib.cpp
)

add_library(whereami STATIC
  deps/whereami/whereami.c
)


add_executable(nes-emu source/main.cpp
source/file_io.cpp source/file_io.h
source/6502.cpp source/6502.h
source/ppu.cpp source/ppu.h
source/cputest.cpp source/cputest.h
source/nes_file.cpp source/nes_file.h
source/mapper.cpp source/mapper.h
source/screen.cpp source/screen.h
source/controller.cpp source/controller.h
source/nes.cpp source/nes.h
source/palette.cpp source/palette.h
source/gui.cpp source/gui.h
source/apu/apu.cpp source/apu/apu.h
source/apu/apu_divider.cpp source/apu/apu_divider.h
source/apu/apu_envelope.cpp source/apu/apu_envelope.h
source/apu/apu_frame_sequencer.cpp source/apu/apu_frame_sequencer.h
source/apu/apu_length_counter.cpp source/apu/apu_length_counter.h
source/apu/apu_sequencer.cpp source/apu/apu_sequencer.h
source/apu/apu_square_channel.cpp source/apu/apu_square_channel.h
source/apu/apu_sweep.cpp source/apu/apu_sweep.h
source/apu/apu_triangle_channel.cpp source/apu/apu_triangle_channel.h
source/apu/apu_noise_channel.cpp source/apu/apu_noise_channel.h
source/apu/apu_delta_modulation_channel.cpp source/apu/apu_delta_modulation_channel.h
source/apu/apu_linear_counter.cpp source/apu/apu_linear_counter.h
source/audiosystem.cpp source/audiosystem.h
source/abstract_mapper.cpp source/abstract_mapper.h
source/mappers/mapper0.cpp source/mappers/mapper0.h
source/mappers/mapper1.cpp source/mappers/mapper1.h
source/mappers/mapper2.cpp source/mappers/mapper2.h
source/mappers/mapper3.cpp source/mappers/mapper3.h
source/mappers/mappers.cpp source/mappers/mappers.h
)

target_link_libraries(nes-emu whereami)
target_link_libraries(nes-emu rtaudio)
target_link_libraries(nes-emu OpenGL::GL)
target_link_libraries(nes-emu imgui)
target_link_libraries(nes-emu glfw)
target_link_libraries(nes-emu GLEW::GLEW)


file(COPY "lib/glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "resources/palette.pal" DESTINATION ${CMAKE_BINARY_DIR})
