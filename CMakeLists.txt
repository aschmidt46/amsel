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
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)


include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(BUILD_WEB TRUE)

if(NOT BUILD_WEB)

include(${CMAKE_SOURCE_DIR}/cmake/desktop_build.cmake)

else()

add_compile_definitions(BUILD_WEB)

include(${CMAKE_SOURCE_DIR}/cmake/wasm_build.cmake)

endif()
