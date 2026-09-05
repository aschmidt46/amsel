# HEADER ONLY LIBRARIES

# Schnellere Konfiguration
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

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

FetchContent_Declare(
  libretro
  GIT_REPOSITORY      https://github.com/libretro/libretro-common
  GIT_TAG             master
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/deps/libretro-common"
)
FetchContent_Populate(libretro)
file(COPY "${PROJECT_SOURCE_DIR}/deps/libretro-common/include/libretro.h" DESTINATION ${SOURCE_INCLUDE_DIR})
