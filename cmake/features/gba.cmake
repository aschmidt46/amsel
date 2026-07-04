
FetchContent_Declare(
  arm_disassembler
  GIT_REPOSITORY  https://github.com/compuphase/ARM-disassembler
  GIT_TAG         main
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/arm_disassembler"
)
FetchContent_MakeAvailable(arm_disassembler)

include_directories(PUBLIC deps/arm_disassembler)


add_library(gba
deps/arm_disassembler/armdisasm.h deps/arm_disassembler/armdisasm.c
source/gba/bus.h source/gba/bus.cpp source/gba/ibus.h
source/gba/arm/bus_types.h source/gba/arm/arm7tdmi.h source/gba/arm/arm7tdmi_types.h
source/gba/arm/arm7tdmi_decoding.cpp
source/gba/arm/arm7tdmi_instructions_arm.cpp
source/gba/arm/arm7tdmi_instructions_thumb.cpp
source/gba/arm/arm7tdmi_processing.cpp
source/gba/arm/arm7tdmi_aux.cpp
source/gba/ppu.h source/gba/ppu_registers.h source/gba/ppu.cpp
source/gba/timer.h
source/gba/timer.cpp
source/gba/gba.h source/gba/gba.cpp
source/gba/test/logging.h source/gba/test/logging.cpp
source/console/console.h
source/console/gba_implementation.h source/console/gba_implementation.cpp
)

target_compile_options(gba PUBLIC -Wall -Wextra -Wpedantic -fsanitize=undefined)

target_link_options(gba PUBLIC -fsanitize=undefined)

target_link_libraries(AMSEL gba)
