
FetchContent_Declare(
  arm_disassembler
  GIT_REPOSITORY  https://github.com/compuphase/ARM-disassembler
  GIT_TAG         main
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/arm_disassembler"
)
FetchContent_MakeAvailable(arm_disassembler)

include_directories(PUBLIC deps/arm_disassembler)

add_library(nds_dasm
  deps/arm_disassembler/armdisasm.h deps/arm_disassembler/armdisasm.c
)


add_library(nds
src/nds/bus.h src/nds/bus.cpp src/gba/ibus.h src/nds/bus_types.h src/nds/bus_debug.cpp
src/nds/nds.h src/nds/nds.cpp
src/nds/bus7.h src/nds/bus7.cpp
src/nds/bus9.h src/nds/bus9.cpp
src/nds/arm/arm946e-s_types.h
src/nds/arm/arm946e-s.h
src/nds/arm/arm946e-s_processing.cpp
src/nds/arm/arm946e-s_instructions_arm.cpp
src/nds/arm/arm946e-s_instructions_thumb.cpp
src/nds/arm/arm946e-s_decoding.cpp
src/nds/arm/arm946e-s_aux.cpp
src/nds/arm/cp15.h
src/nds/arm/cp15.cpp

src/gba/register/general_purpose.h src/gba/register/general_purpose.cpp
src/gba/arm/bus_types.h src/gba/arm/arm7tdmi.h src/gba/arm/arm7tdmi_types.h
src/gba/arm/arm7tdmi_decoding.cpp
src/gba/arm/arm7tdmi_instructions_arm.cpp
src/gba/arm/arm7tdmi_instructions_thumb.cpp
src/gba/arm/arm7tdmi_processing.cpp
src/gba/arm/arm7tdmi_aux.cpp

src/gba/scheduler.h
src/nds/scheduler.h src/nds/scheduler.cpp

src/nds/lcd/2d/ppu.h
src/nds/lcd/2d/ppu_registers.h
src/nds/lcd/2d/ppu_rendering.cpp
src/nds/lcd/2d/ppu_io.cpp
src/nds/lcd/3d/gpu.h
src/nds/lcd/lcd.h
src/nds/lcd/lcd.cpp
src/nds/lcd/lcd_banks.cpp

src/nds/peripheral/dma.h
src/nds/peripheral/dma.cpp
src/nds/peripheral/timer.h
src/nds/peripheral/timer.cpp
src/nds/peripheral/ipc.h
src/nds/peripheral/ipc.cpp
src/nds/peripheral/maths.h
src/nds/peripheral/maths.cpp
src/nds/peripheral/cartridge.h
src/nds/peripheral/cartridge.cpp
src/nds/peripheral/spi.h
src/nds/peripheral/spi.cpp
src/nds/peripheral/firmware.h
src/nds/peripheral/firmware.cpp
src/nds/peripheral/touch.h
src/nds/peripheral/touch.cpp

src/console/console.h
src/console/nds_implementation.h src/console/nds_implementation.cpp
)

target_compile_options(nds PUBLIC -Wall)

target_link_libraries(nds PUBLIC nds_dasm)


target_link_libraries(AMSEL nds)
