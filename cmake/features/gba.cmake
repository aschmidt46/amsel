
FetchContent_Declare(
  arm_disassembler
  GIT_REPOSITORY  https://github.com/compuphase/ARM-disassembler
  GIT_TAG         main
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/arm_disassembler"
)
FetchContent_MakeAvailable(arm_disassembler)

include_directories(PUBLIC deps/arm_disassembler)

add_library(gba_dasm
  deps/arm_disassembler/armdisasm.h deps/arm_disassembler/armdisasm.c
)


add_library(gba
src/gba/bus.h src/gba/bus.cpp src/gba/ibus.h
src/gba/register/general_purpose.h src/gba/register/general_purpose.cpp
src/gba/arm/bus_types.h src/gba/arm/arm7tdmi.h src/gba/arm/arm7tdmi_types.h
src/gba/arm/arm7tdmi_decoding.cpp
src/gba/arm/arm7tdmi_instructions_arm.cpp
src/gba/arm/arm7tdmi_instructions_thumb.cpp
src/gba/arm/arm7tdmi_processing.cpp
src/gba/arm/arm7tdmi_aux.cpp
src/gba/ppu.h src/gba/ppu_registers.h src/gba/ppu.cpp
src/gba/timer.h
src/gba/timer.cpp
src/gba/dma.h
src/gba/dma.cpp
src/gba/gba.h src/gba/gba.cpp
src/gba/test/logging.h src/gba/test/logging.cpp
src/console/console.h
src/console/gba_implementation.h src/console/gba_implementation.cpp
)

target_compile_options(gba PUBLIC -Wall -Wextra -Wpedantic)

target_link_libraries(gba PUBLIC gba_dasm)

set_property(TARGET gba PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")


target_link_libraries(AMSEL gba)
