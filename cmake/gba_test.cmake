set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")


include_directories(PUBLIC include)
include_directories(PUBLIC include_deps)
include_directories(PUBLIC .)
include_directories(PUBLIC source)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami/src)
include_directories(PUBLIC deps/cereal/include)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY  https://github.com/google/googletest
    GIT_TAG         v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(googletest)



add_library(gba
source/gba/bus.h source/gba/bus.cpp source/gba/ibus.h
source/gba/arm/bus_types.h source/gba/arm/arm7tdmi.h
source/gba/arm/arm7tdmi_decoding.cpp
source/gba/arm/arm7tdmi_instructions_arm.cpp
source/gba/arm/arm7tdmi_instructions_thumb.cpp
source/gba/arm/arm7tdmi_processing.cpp
source/gba/arm/arm7tdmi_aux.cpp
)
target_compile_options(gba PUBLIC -Wall -Wextra -Werror -Wpedantic)

add_executable(gba-test source/gba/main.cpp)
target_link_libraries(gba-test gba)

enable_testing()

add_executable(
  gba_tests
  source/gba/test/logging.h source/gba/test/logging.cpp
  source/gba/test/test.cpp
  source/gba/test/testbus.cpp
  source/gba/test/testbus.h
  source/gba/test/cpustate.h
)
target_compile_options(gba_tests PUBLIC -Wall -Wextra -Werror -Wpedantic)
target_link_libraries(
  gba_tests
  gba
  GTest::gtest_main
)

include(GoogleTest)
# set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(gba_tests)

