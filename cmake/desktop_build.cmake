set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

if(WIN32)
message("-------- Detected OS: Windows")
add_compile_definitions(NES_ON_WINDOWS)
endif()

#Kein Terminal:
if(RELEASE)
message("------------- Building Windows release build")
add_link_options(-mwindows)
endif()


include_directories(PUBLIC include)
include_directories(PUBLIC include_deps)
include_directories(PUBLIC .)
include_directories(PUBLIC source)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami/src)
include_directories(PUBLIC deps/cereal/include)


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
  deps/whereami/src/whereami.c
)

# Rust
# set(RUSTFLAGS -Awarnings)
corrosion_import_crate(MANIFEST_PATH source/cgb/Cargo.toml)
corrosion_add_cxxbridge(rusty_bridge CRATE cgbcore FILES bridge.rs)

add_executable(AMSEL source/main.cpp
source/framework/common.cpp source/framework/common.h
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
source/console/console.h
source/console/nes_implementation.h source/console/nes_implementation.cpp
source/console/cgb_implementation.h source/console/cgb_implementation.cpp
source/console/console.cpp
source/framework/locale.h
source/framework/locale.cpp
source/console/dummy_implementation.h
source/framework/glm_replacement.h
)

# add_executable(gba-test source/gba/main.cpp source/gba/bus.h source/gba/arm/bus_types.h source/gba/arm/arm7tdmi.h source/gba/arm/arm7tdmi_decoding.cpp source/gba/arm/arm7tdmi_instructions.cpp)

target_link_libraries(AMSEL whereami)
target_link_libraries(AMSEL rtaudio)
target_link_libraries(AMSEL imgui)

set(BUILD_SHARED_LIBS FALSE)
glad_add_library(glad_gl_core_46 STATIC API gl:core=4.6)
target_link_libraries(AMSEL glad_gl_core_46)
target_link_libraries(AMSEL glfw)

target_link_libraries(AMSEL cgbcore)
target_link_libraries(AMSEL rusty_bridge)


file(COPY "resources/nes/palette.pal" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/source/framework/locales DESTINATION ${CMAKE_BINARY_DIR}/)

if(RELEASE)
  add_custom_command(TARGET AMSEL POST_BUILD
        COMMAND powershell.exe "${PROJECT_SOURCE_DIR}/scripts/build_release.ps1")
  add_custom_command(TARGET AMSEL POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                   "Built Windows Release for x64 in ./release")
endif()



# TESTS

message("Preparing files for testing...")
FetchContent_Declare(
  gb-test-roms
  GIT_REPOSITORY https://github.com/retrio/gb-test-roms.git
  GIT_TAG        master
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/source/cgb/resources/gb-test-roms-master"
  )
FetchContent_MakeAvailable(gb-test-roms)