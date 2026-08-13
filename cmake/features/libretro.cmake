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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# add_library(skeleton SHARED
# src/libretro/libretro.cpp
# )

add_library(AMSEL SHARED
src/framework/common.cpp src/framework/common.h
src/framework/global_web.cpp
src/console/console.h
src/console/console.cpp
src/console/dummy_implementation.h
src/libretro/libretro_implementation.cpp
)

set_property(TARGET AMSEL PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")


# Give the code a way of determining that it is targeting libretro.
target_compile_definitions(AMSEL PRIVATE __LIBRETRO__)

# Allow only the libretro API functions to be exported.
set_target_properties(AMSEL PROPERTIES C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

# Adjust the library's filename. This is relied upon by libretro's CI infrastructure.

# Remove the 'lib' prefix.
set_target_properties(AMSEL PROPERTIES PREFIX "")

# Append the given suffix.
set_target_properties(AMSEL PROPERTIES OUTPUT_NAME "AMSEL${LIBRETRO_SUFFIX}")


