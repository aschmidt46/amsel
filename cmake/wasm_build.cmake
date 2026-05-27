set(CMAKE_CXX_FLAGS_DEBUG "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories(PUBLIC include)
include_directories(PUBLIC include_deps)
include_directories(PUBLIC .)
include_directories(PUBLIC source)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami/src)
include_directories(PUBLIC deps/cereal/include)

set(Rust_CARGO_TARGET wasm32-unknown-emscripten)
set(Rust_RUSTUP_INSTALL_MISSING_TARGET ON)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Corrosion)

# Rust
# set(RUSTFLAGS -Awarnings)
corrosion_import_crate(MANIFEST_PATH source/cgb/Cargo.toml PROFILE release)
corrosion_add_cxxbridge(rusty_bridge CRATE cgbcore FILES bridge.rs)


add_executable(AMSEL-web
source/framework/common.cpp source/framework/common.h
source/nes/6502.cpp source/nes/6502.h
source/nes/ppu.cpp source/nes/ppu.h
source/nes/nes_file.cpp source/nes/nes_file.h
source/nes/mapper.cpp source/nes/mapper.h
source/nes/controller.cpp source/nes/controller.h
source/nes/nes.cpp source/nes/nes.h
source/nes/palette.cpp source/nes/palette.h
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
source/framework/global_web.cpp
)
target_compile_options(AMSEL-web PUBLIC -Wall -Wextra -Werror -Wpedantic)

set_target_properties(AMSEL-web PROPERTIES LINK_FLAGS "--bind --emit-tsd AMSEL-web.d.ts -s TOTAL_STACK=512mb -s EXPORT_ES6=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 -s SINGLE_FILE=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 -O3 -s WASM=1 -Wall -s MODULARIZE=1")

target_link_libraries(AMSEL-web cgbcore)
target_link_libraries(AMSEL-web rusty_bridge)

set(WEB_BASE "${PROJECT_SOURCE_DIR}/source/web/amsel-web")

# file(COPY "${PROJECT_BINARY_DIR}/AMSEL-web.js" DESTINATION "${WEB_BASE}/src/emscripten/")
# file(COPY "${PROJECT_BINARY_DIR}/AMSEL-web.d.ts" DESTINATION "${WEB_BASE}/src/emscripten/")

add_custom_command(
        TARGET AMSEL-web POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/AMSEL-web.js
                ${WEB_BASE}/src/emscripten/)

add_custom_command(
        TARGET AMSEL-web POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/AMSEL-web.d.ts
                ${WEB_BASE}/src/emscripten/)

file(COPY "${CMAKE_SOURCE_DIR}/source/framework/locales" DESTINATION "${WEB_BASE}/src/assets")


