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

add_compile_definitions(_DEBUG)

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)
#Kein Terminal:
# add_link_options(-mwindows)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include_directories(PUBLIC include)

find_package(glfw3 REQUIRED)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)

set(RTAUDIO_BUILD_STATIC_LIBS TRUE)
add_subdirectory(deps/rtaudio)

include_directories(PUBLIC .)
include_directories(PUBLIC source)
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
source/framework/file_io.cpp source/framework/file_io.h
source/framework/input.cpp source/framework/input.h
source/framework/windowing.cpp source/framework/windowing.h
source/nes/6502.cpp source/nes/6502.h
source/nes/ppu.cpp source/nes/ppu.h
source/nes/nes_file.cpp source/nes/nes_file.h
source/nes/mapper.cpp source/nes/mapper.h
source/framework/screen.cpp source/framework/screen.h
source/nes/controller.cpp source/nes/controller.h
source/nes/nes.cpp source/nes/nes.h
source/nes/palette.cpp source/nes/palette.h
source/gui.cpp source/gui.h
source/nes/apu/apu.cpp source/nes/apu/apu.h
source/nes/apu/apu_divider.cpp source/nes/apu/apu_divider.h
source/nes/apu/apu_envelope.cpp source/nes/apu/apu_envelope.h
source/nes/apu/apu_frame_sequencer.cpp source/nes/apu/apu_frame_sequencer.h
source/nes/apu/apu_length_counter.cpp source/nes/apu/apu_length_counter.h
source/nes/apu/apu_sequencer.cpp source/nes/apu/apu_sequencer.h
source/nes/apu/apu_square_channel.cpp source/nes/apu/apu_square_channel.h
source/nes/apu/apu_sweep.cpp source/nes/apu/apu_sweep.h
source/nes/apu/apu_triangle_channel.cpp source/nes/apu/apu_triangle_channel.h
source/nes/apu/apu_noise_channel.cpp source/nes/apu/apu_noise_channel.h
source/nes/apu/apu_delta_modulation_channel.cpp source/nes/apu/apu_delta_modulation_channel.h
source/nes/apu/apu_linear_counter.cpp source/nes/apu/apu_linear_counter.h
source/framework/audiosystem.cpp source/framework/audiosystem.h
source/nes/mappers/abstract_mapper.cpp source/nes/mappers/abstract_mapper.h
source/nes/mappers/mapper0.cpp source/nes/mappers/mapper0.h
source/nes/mappers/mapper1.cpp source/nes/mappers/mapper1.h
source/nes/mappers/mapper2.cpp source/nes/mappers/mapper2.h
source/nes/mappers/mapper3.cpp source/nes/mappers/mapper3.h
source/nes/mappers/mapper4.cpp source/nes/mappers/mapper4.h
source/nes/mappers/mapper7.cpp source/nes/mappers/mapper7.h
source/nes/mappers/mappers.cpp source/nes/mappers/mappers.h
)

target_link_libraries(nes-emu whereami)
target_link_libraries(nes-emu rtaudio)
target_link_libraries(nes-emu OpenGL::GL)
target_link_libraries(nes-emu imgui)
target_link_libraries(nes-emu glfw)
target_link_libraries(nes-emu GLEW::GLEW)


file(COPY "lib/glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "resources/palette.pal" DESTINATION ${CMAKE_BINARY_DIR})
