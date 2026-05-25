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



add_executable(gba-test source/gba/main.cpp
source/gba/bus.h source/gba/bus.cpp
source/gba/arm/bus_types.h source/gba/arm/arm7tdmi.h
source/gba/arm/arm7tdmi_decoding.cpp
source/gba/arm/arm7tdmi_instructions_arm.cpp
source/gba/arm/arm7tdmi_instructions_thumb.cpp
source/gba/arm/arm7tdmi_processing.cpp
)

