
include_directories(PUBLIC include)
include_directories(PUBLIC include_deps)
include_directories(PUBLIC .)
include_directories(PUBLIC source)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami/src)
include_directories(PUBLIC deps/cereal/include)
include_directories(PUBLIC deps/arm_disassembler)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY  https://github.com/google/googletest
    GIT_TAG         v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  AMSEL
  source/gba/test/logging.h source/gba/test/logging.cpp
  source/gba/test/test.cpp
  source/gba/test/testbus.cpp
  source/gba/test/testbus.h
  source/gba/test/cpustate.h
)
target_compile_options(AMSEL PUBLIC -Wall -Wextra -Wpedantic)
target_link_libraries(
  AMSEL
  GTest::gtest_main
)

include(GoogleTest)
# set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(AMSEL)

