cmake_minimum_required(VERSION 3.20)

# C++ Laufzeitbibliothek statisch verlinken auf MSVC
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

project(antons-AMSEL)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
# set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)


include(FetchContent)
file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/deps")
message("Fetching headers...")
include(${CMAKE_SOURCE_DIR}/cmake/fetch_headers.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

option(FEATURE_WEB "Build WebAssembly for Web-App" OFF)
option(FEATURE_DESKTOP "Build Desktop-App" ON)

option(FEATURE_TEST_SUITE "GBA Tests" OFF)

option(FEATURE_NES "NES-Core" ON)
option(FEATURE_CGB "CGB-Core" ON)
option(FEATURE_GBA "GBA-Core" ON)

include(CMakeDependentOption)

cmake_dependent_option(FEATURE_DEBUGGER "Add Debugger to Desktop-App" ON FEATURE_DESKTOP OFF)

if(FEATURE_DEBUGGER)
  add_compile_definitions(FEATURE_DEBUGGER)
endif()


if(FEATURE_TEST_SUITE)

  include(${CMAKE_SOURCE_DIR}/cmake/gba_test.cmake)

else()

  if(FEATURE_DESKTOP)

    message("Fetching dependencies...")
    include(${CMAKE_SOURCE_DIR}/cmake/fetch_dependencies.cmake)

    include(${CMAKE_SOURCE_DIR}/cmake/features/desktop.cmake)

  else()

    add_compile_definitions(BUILD_WEB)

    include(${CMAKE_SOURCE_DIR}/cmake/features/web.cmake)

  endif()

  
  if(FEATURE_NES)
  add_compile_definitions(BUILD_NES)
  include(${CMAKE_SOURCE_DIR}/cmake/features/nes.cmake)
  endif()

  if(FEATURE_CGB)
  add_compile_definitions(BUILD_CGB)
  include(${CMAKE_SOURCE_DIR}/cmake/features/cgb.cmake)
  endif()

  if(FEATURE_GBA)
  add_compile_definitions(BUILD_GBA)
  include(${CMAKE_SOURCE_DIR}/cmake/features/gba.cmake)
  endif()

endif()