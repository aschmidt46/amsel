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
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v2.0.8
  SOURCE_SUBDIR	 cmake
)

FetchContent_MakeAvailable(glad)


# HEADER ONLY LIBRARIES

set(SOURCE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include_deps)

FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG        v1.0.5
  SOURCE_DIR  "${PROJECT_SOURCE_DIR}/deps/concurrentqueue"
)

FetchContent_Populate(concurrentqueue)
file(COPY "${PROJECT_SOURCE_DIR}/deps/concurrentqueue/concurrentqueue.h" DESTINATION ${SOURCE_INCLUDE_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/deps/concurrentqueue/blockingconcurrentqueue.h" DESTINATION ${SOURCE_INCLUDE_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/deps/concurrentqueue/lightweightsemaphore.h" DESTINATION ${SOURCE_INCLUDE_DIR})


FetchContent_Declare(
  imgui-notify
  GIT_REPOSITORY      https://github.com/TyomaVader/ImGuiNotify
  GIT_TAG             d00e45f
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/imgui-notify"
)
FetchContent_Populate(imgui-notify)
file(COPY "${PROJECT_SOURCE_DIR}/deps/imgui-notify/unixExample/backends/ImGuiNotify.hpp" DESTINATION ${SOURCE_INCLUDE_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/deps/imgui-notify/win32Example/fonts/IconsFontAwesome6.h" DESTINATION ${SOURCE_INCLUDE_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/deps/imgui-notify/win32Example/fonts/fa-solid-900.h" DESTINATION ${SOURCE_INCLUDE_DIR})

FetchContent_Declare(
  inicpp
  GIT_REPOSITORY      https://github.com/Rookfighter/inifile-cpp/
  GIT_TAG             7e49789
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/inicpp"
)
FetchContent_Populate(inicpp)
file(COPY "${PROJECT_SOURCE_DIR}/deps/inicpp/include/inicpp.h" DESTINATION ${SOURCE_INCLUDE_DIR})

FetchContent_Declare(
  pfd
  GIT_REPOSITORY      https://github.com/samhocevar/portable-file-dialogs/
  GIT_TAG             c12ea8c
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/pfd"
)
FetchContent_Populate(pfd)
file(COPY "${PROJECT_SOURCE_DIR}/deps/pfd/portable-file-dialogs.h" DESTINATION ${SOURCE_INCLUDE_DIR})

FetchContent_Declare(
  nlohmann
  GIT_REPOSITORY      https://github.com/nlohmann/json
  GIT_TAG             v3.12.0
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/nlohmann"
)
FetchContent_Populate(nlohmann)
file(COPY "${PROJECT_SOURCE_DIR}/deps/nlohmann/single_include/nlohmann/json.hpp" DESTINATION ${SOURCE_INCLUDE_DIR}/nlohmann)
file(COPY "${PROJECT_SOURCE_DIR}/deps/nlohmann/single_include/nlohmann/json_fwd.hpp" DESTINATION ${SOURCE_INCLUDE_DIR}/nlohmann)


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