add_library(nes
src/nes/6502.cpp src/nes/6502.h
src/nes/ppu.cpp src/nes/ppu.h
src/nes/nes_file.cpp src/nes/nes_file.h
src/nes/mapper.cpp src/nes/mapper.h
src/nes/controller.cpp src/nes/controller.h
src/nes/nes.cpp src/nes/nes.h
src/nes/palette.cpp src/nes/palette.h
src/nes/apu/apu.cpp src/nes/apu/apu.h
src/nes/apu/apu_divider.cpp src/nes/apu/apu_divider.h
src/nes/apu/apu_envelope.cpp src/nes/apu/apu_envelope.h
src/nes/apu/apu_frame_sequencer.cpp src/nes/apu/apu_frame_sequencer.h
src/nes/apu/apu_length_counter.cpp src/nes/apu/apu_length_counter.h
src/nes/apu/apu_sequencer.cpp src/nes/apu/apu_sequencer.h
src/nes/apu/apu_square_channel.cpp src/nes/apu/apu_square_channel.h
src/nes/apu/apu_sweep.cpp src/nes/apu/apu_sweep.h
src/nes/apu/apu_triangle_channel.cpp src/nes/apu/apu_triangle_channel.h
src/nes/apu/apu_noise_channel.cpp src/nes/apu/apu_noise_channel.h
src/nes/apu/apu_delta_modulation_channel.cpp src/nes/apu/apu_delta_modulation_channel.h
src/nes/apu/apu_linear_counter.cpp src/nes/apu/apu_linear_counter.h
src/nes/mappers/abstract_mapper.cpp src/nes/mappers/abstract_mapper.h
src/nes/mappers/mapper0.cpp src/nes/mappers/mapper0.h
src/nes/mappers/mapper1.cpp src/nes/mappers/mapper1.h
src/nes/mappers/mapper2.cpp src/nes/mappers/mapper2.h
src/nes/mappers/mapper3.cpp src/nes/mappers/mapper3.h
src/nes/mappers/mapper4.cpp src/nes/mappers/mapper4.h
src/nes/mappers/mapper7.cpp src/nes/mappers/mapper7.h
src/nes/mappers/mappers.cpp src/nes/mappers/mappers.h
)

target_compile_options(nes PUBLIC -Wall -Wextra -Wpedantic)

set_property(TARGET nes PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

add_library(nes_implementation src/console/console.h
src/console/nes_implementation.h src/console/nes_implementation.cpp
)

set_property(TARGET nes_implementation PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

target_link_libraries(AMSEL nes nes_implementation)

file(COPY "resources/nes/palette.pal" DESTINATION ${CMAKE_BINARY_DIR})
