cmake_minimum_required(VERSION 3.20)

# C++ Laufzeitbibliothek statisch verlinken auf MSVC
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

project(antons-AMSEL)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)


include(FetchContent)
file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/deps")
message("Fetching headers...")
include(${CMAKE_SOURCE_DIR}/cmake/fetch_headers.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

option(BUILD_WEB "Build WebAssembly for Web-App" OFF)


if(NOT BUILD_WEB)

message("Fetching dependencies...")
include(${CMAKE_SOURCE_DIR}/cmake/fetch_dependencies.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/desktop_build.cmake)

else()

add_compile_definitions(BUILD_WEB)

include(${CMAKE_SOURCE_DIR}/cmake/wasm_build.cmake)

endif()
