
include_directories(PUBLIC include)
include_directories(PUBLIC include_deps)
include_directories(PUBLIC .)
include_directories(PUBLIC source)
include_directories(PUBLIC deps/imgui)
include_directories(PUBLIC deps/imgui/backends)
include_directories(PUBLIC deps/rtaudio)
include_directories(PUBLIC deps/whereami/src)
include_directories(PUBLIC deps/cereal/include)


add_executable(AMSEL
source/framework/common.cpp source/framework/common.h
source/console/console.h
source/console/console.cpp
source/framework/locale.h
source/framework/locale.cpp
source/console/dummy_implementation.h
source/framework/global_web.cpp
)
target_compile_options(AMSEL PUBLIC -Wall -Wextra -Wpedantic -Werror)

# target_compile_options(AMSEL PUBLIC -Wall -Wextra -Wpedantic -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-merge)

# target_link_options(AMSEL PUBLIC -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-merge)

set_target_properties(AMSEL PROPERTIES LINK_FLAGS "--bind --emit-tsd AMSEL.d.ts -s TOTAL_STACK=512mb -s EXPORT_ES6=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 -s SINGLE_FILE=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 -O3 -s WASM=1 -Wall -s MODULARIZE=1")


set(WEB_BASE "${PROJECT_SOURCE_DIR}/source/web/amsel-web")

make_directory(${WEB_BASE}/src/emscripten)

add_custom_command(
        TARGET AMSEL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/AMSEL.js
                ${WEB_BASE}/src/emscripten/)

add_custom_command(
        TARGET AMSEL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/AMSEL.d.ts
                ${WEB_BASE}/src/emscripten/)

file(COPY "${CMAKE_SOURCE_DIR}/source/framework/locales" DESTINATION "${WEB_BASE}/src/assets")


