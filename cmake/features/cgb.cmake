
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
set(RUST_FEATURES "")
if(FEATURE_GAMEBOY_CGB_SUPPORT)
add_compile_definitions(FEATURE_CGB)
set(RUST_FEATURES "${RUST_FEATURES}cgb ")
endif()
if(FEATURE_LIBRETRO_CORE)
set(RUST_FEATURES "${RUST_FEATURES}libretro")
endif()


corrosion_import_crate(MANIFEST_PATH src/cgb/Cargo.toml PROFILE release FEATURES "${RUST_FEATURES}")


corrosion_add_cxxbridge(rusty_bridge CRATE cgbcore FILES bridge.rs)
include_directories(${CMAKE_BINARY_DIR}/corrosion_generated/cxxbridge/rusty_bridge/include)

add_library(cgb_implementation src/console/console.h
src/console/cgb_implementation.h src/console/cgb_implementation.cpp
)

set_property(TARGET rusty_bridge PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

set_property(TARGET cgb_implementation PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

target_link_libraries(cgb_implementation cgbcore rusty_bridge)


# TESTS
if(FEATURE_TEST_SUITE)

    message("Preparing files for testing...")
    FetchContent_Declare(
        gb-test-roms
        GIT_REPOSITORY https://github.com/retrio/gb-test-roms.git
        GIT_TAG        master
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/cgb/resources/gb-test-roms-master"
    )
    FetchContent_MakeAvailable(gb-test-roms)

    add_test(NAME "Gameboy Tests (Cargo)" COMMAND cargo test --manifest-path ${CMAKE_SOURCE_DIR}/src/cgb/Cargo.toml)
    
else()
    target_link_libraries(AMSEL cgb_implementation)
endif()
