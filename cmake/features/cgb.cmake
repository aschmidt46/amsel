
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
set(RUST_FEATURES "${RUST_FEATURES}f_cgb ")
endif()
if(FEATURE_LIBRETRO_CORE)
set(RUST_FEATURES "${RUST_FEATURES}libretro")
endif()
if(FEATURE_TEST_SUITE)
set(RUST_FEATURES "${RUST_FEATURES}f_test")
endif()

# In Debug builds, we are forced to use .dll due to incompatible _ITERATOR_DEBUG_LEVEL values: 0 in cc-rs via Rust and 2 in C++.
# if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#   set(RECURSIVE_CPP_CRATE_TYPE cdylib)
# else()
#   set(RECURSIVE_CPP_CRATE_TYPE staticlib)
# endif()

corrosion_import_crate(MANIFEST_PATH src/cgb/Cargo.toml FEATURES "${RUST_FEATURES}")

corrosion_add_cxxbridge(rusty_bridge CRATE cgbcore FILES bridge.rs)
# corrosion_set_env_vars(rusty_bridge "CFLAGS=-MTd" "CXXFLAGS=-MTd")
include_directories(${CMAKE_BINARY_DIR}/corrosion_generated/cxxbridge/rusty_bridge/include)

add_library(cgb_implementation src/console/console.h
src/console/cgb_implementation.h src/console/cgb_implementation.cpp
src/console/cgb_bridge.h src/console/cgb_bridge.cpp
)

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

    add_test(NAME "Gameboy Tests (Cargo)" COMMAND cargo test --manifest-path ${CMAKE_SOURCE_DIR}/src/cgb/Cargo.toml --features "${RUST_FEATURES}")

else()
    target_link_libraries(AMSEL cgb_implementation)
endif()
