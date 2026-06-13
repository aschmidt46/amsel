
# Schnellere Konfiguration
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Corrosion)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.5-docking # release-1.10.0
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/imgui"
  )
FetchContent_MakeAvailable(imgui)
  
set(RTAUDIO_BUILD_STATIC_LIBS TRUE)
FetchContent_Declare(
  rtaudio
  GIT_REPOSITORY  https://github.com/thestk/rtaudio.git
  GIT_TAG         6.0.1
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/rtaudio"
)
FetchContent_MakeAvailable(rtaudio)

FetchContent_Declare(
  whereami
  GIT_REPOSITORY  https://github.com/gpakosz/whereami.git
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/whereami"
)
FetchContent_MakeAvailable(whereami)

set(GLFW_LIBRARY_TYPE STATIC)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY  https://github.com/glfw/glfw
  GIT_TAG         3.4
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/glfw"
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  arm_disassembler
  GIT_REPOSITORY  https://github.com/compuphase/ARM-disassembler
  GIT_TAG         main
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/arm_disassembler"
)
FetchContent_MakeAvailable(arm_disassembler)


FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v2.0.8
  SOURCE_SUBDIR	 cmake
  )
  
  FetchContent_MakeAvailable(glad)


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