
if(FEATURE_WEB)
    set(Rust_CARGO_TARGET wasm32-unknown-emscripten)
endif()

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Corrosion)

find_program(CXXBRIDGE cxxbridge PATHS "$ENV{HOME}/.cargo/bin/")
if (CXXBRIDGE STREQUAL "CXXBRIDGE-NOTFOUND")
    message("Could not find cxxbridge, trying to install with `cargo install cxxbridge-cmd'")
    find_program(CARGO cargo PATHS "$ENV{HOME}/.cargo/bin/")
    if (CARGO STREQUAL "CARGO-NOTFOUND")
        message(FATAL_ERROR "Requires cargo available in path, install via rustup https://rustup.rs/")
    endif()
    execute_process(COMMAND ${CARGO} install cxxbridge-cmd)
    find_program(CXXBRIDGE cxxbridge PATHS "$ENV{HOME}/.cargo/bin/")
endif()


# Rust
# set(RUSTFLAGS -Awarnings)
corrosion_import_crate(MANIFEST_PATH source/cgb/Cargo.toml)
corrosion_add_cxxbridge(rusty_bridge CRATE cgbcore FILES bridge.rs)
include_directories(${CMAKE_BINARY_DIR}/corrosion_generated/cxxbridge/rusty_bridge/include)

add_library(cgb_implementation source/console/console.h
source/console/cgb_implementation.h source/console/cgb_implementation.cpp
)

target_link_libraries(cgb_implementation cgbcore rusty_bridge)
target_link_libraries(AMSEL cgb_implementation)
