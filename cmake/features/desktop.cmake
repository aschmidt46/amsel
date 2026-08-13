if(WIN32)
message("-------- Detected OS: Windows")
add_compile_definitions(NES_ON_WINDOWS)
endif()

#Kein Terminal:
if(NOT FEATURE_CONSOLE)
message("-------------> Building Windows release build")
add_link_options(-mwindows)
endif()

# Abhängigkeiten:

message("Fetching dependencies...")

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
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY  https://github.com/glfw/glfw
  GIT_TAG         3.4
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/glfw"
)
FetchContent_MakeAvailable(glfw)


FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v2.0.8
  SOURCE_SUBDIR	 cmake
  )
  
  FetchContent_MakeAvailable(glad)


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


add_executable(AMSEL src/main.cpp
src/framework/common.cpp src/framework/common.h
src/framework/file_io.cpp src/framework/file_io.h
src/framework/input.cpp src/framework/input.h
src/framework/windowing.cpp src/framework/windowing.h
src/framework/screen.cpp src/framework/screen.h
src/gui.cpp src/gui.h
src/framework/audiosystem.cpp src/framework/audiosystem.h
src/console/console.h
src/console/console.cpp
src/framework/locale.h
src/framework/locale.cpp
src/console/dummy_implementation.h
src/framework/vector.h
)

set_property(TARGET AMSEL PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")


# target_compile_options(AMSEL PUBLIC -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-merge)
# target_link_options(AMSEL PUBLIC -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-merge)

target_compile_options(AMSEL PUBLIC -Wall -Wextra -Wpedantic)

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

make_directory(${CMAKE_BINARY_DIR}/locales/)

add_custom_command(
        TARGET AMSEL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/src/framework/locales/de.json
                ${CMAKE_BINARY_DIR}/locales/de.json)

add_custom_command(
        TARGET AMSEL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/src/framework/locales/en.json
                ${CMAKE_BINARY_DIR}/locales/en.json)

if(RELEASE)
  add_custom_command(TARGET AMSEL POST_BUILD
        COMMAND powershell.exe "${PROJECT_SOURCE_DIR}/scripts/build_release.ps1")
  add_custom_command(TARGET AMSEL POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                   "Built Windows Release for x64 in ./release")
endif()
