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




add_library(AMSEL
source/framework/common.cpp source/framework/common.h
source/console/console.h
source/console/console.cpp
source/console/dummy_implementation.h
source/framework/glm_replacement.h
source/libretro/libretro_implementation.cpp
)

