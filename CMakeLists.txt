cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    antons-nes-emu
    VERSION 0.1.0
    DESCRIPTION "Nes-Emulator"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    antons-nes-emu_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    antons-nes-emu_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(antons-nes-emu_lib PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(antons-nes-emu_exe source/main.cpp)
add_executable(antons-nes-emu::exe ALIAS antons-nes-emu_exe)

set_property(TARGET antons-nes-emu_exe PROPERTY OUTPUT_NAME antons-nes-emu)

target_compile_features(antons-nes-emu_exe PRIVATE cxx_std_20)

target_link_libraries(antons-nes-emu_exe PRIVATE antons-nes-emu_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT antons-nes-emu_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of antons-nes-emu"
  )
endif()

include(cmake/dev-mode.cmake)
