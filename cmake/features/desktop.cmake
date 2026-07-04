set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

if(WIN32)
message("-------- Detected OS: Windows")
add_compile_definitions(NES_ON_WINDOWS)
endif()

#Kein Terminal:
if(RELEASE)
message("------------- Building Windows release build")
add_link_options(-mwindows)
endif()

include_directories(PUBLIC include_deps)
include_directories(PUBLIC include)
include_directories(PUBLIC .)
include_directories(PUBLIC source)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami/src)
include_directories(PUBLIC deps/cereal/include)


add_library(imgui STATIC
  deps/imgui/imgui.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/imgui_widgets.cpp
  deps/imgui/imgui_tables.cpp
  deps/imgui/backends/imgui_impl_glfw.cpp
  deps/imgui/backends/imgui_impl_opengl3.cpp
  deps/imgui/misc/cpp/imgui_stdlib.cpp
)

add_library(whereami STATIC
  deps/whereami/src/whereami.c
)


add_executable(AMSEL source/main.cpp
source/framework/common.cpp source/framework/common.h
source/framework/file_io.cpp source/framework/file_io.h
source/framework/input.cpp source/framework/input.h
source/framework/windowing.cpp source/framework/windowing.h
source/framework/screen.cpp source/framework/screen.h
source/gui.cpp source/gui.h
source/framework/audiosystem.cpp source/framework/audiosystem.h
source/console/console.h
source/console/console.cpp
source/framework/locale.h
source/framework/locale.cpp
source/console/dummy_implementation.h
source/framework/glm_replacement.h
)

target_compile_options(AMSEL PUBLIC -Wall -Wextra -Wpedantic)

# target_compile_options(AMSEL PUBLIC -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-merge)
# target_link_options(AMSEL PUBLIC -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-merge)

#target_compile_options(AMSEL PUBLIC -Wall -Wextra -Wpedantic)

target_link_libraries(AMSEL whereami)
target_link_libraries(AMSEL rtaudio)
target_link_libraries(AMSEL imgui)

set(BUILD_SHARED_LIBS FALSE)
glad_add_library(glad_gl_core_46 STATIC API gl:core=4.6)
target_link_libraries(AMSEL glad_gl_core_46)
target_link_libraries(AMSEL glfw)

if(UNIX AND NOT APPLE)
target_link_libraries(AMSEL "-lX11")
endif()



add_custom_command(
        TARGET AMSEL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/source/framework/locales/*.json
                ${CMAKE_BINARY_DIR}/locales/)

if(RELEASE)
  add_custom_command(TARGET AMSEL POST_BUILD
        COMMAND powershell.exe "${PROJECT_SOURCE_DIR}/scripts/build_release.ps1")
  add_custom_command(TARGET AMSEL POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                   "Built Windows Release for x64 in ./release")
endif()



# TESTS

message("Preparing files for testing...")
FetchContent_Declare(
  gb-test-roms
  GIT_REPOSITORY https://github.com/retrio/gb-test-roms.git
  GIT_TAG        master
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/source/cgb/resources/gb-test-roms-master"
  )
FetchContent_MakeAvailable(gb-test-roms)