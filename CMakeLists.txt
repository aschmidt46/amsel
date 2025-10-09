cmake_minimum_required(VERSION 3.20)

# C++ Laufzeitbibliothek statisch verlinken auf MSVC
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

project(antons-nes-emu)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ggdb")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include_directories(PUBLIC include)

find_package(GLFW3 REQUIRED)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)



include_directories(PUBLIC .)



add_library(cpu source/6502.cpp source/6502.h)
add_library(cputest source/cputest.cpp source/cputest.h)
add_library(nesfile source/nes_file.cpp source/nes_file.h)
add_library(mapper source/mapper.cpp source/mapper.h)
add_library(screen source/screen.cpp source/screen.h)



add_executable(nes-emu source/main.cpp)
target_link_libraries(nes-emu cputest nesfile cpu mapper screen)
target_link_libraries(nes-emu OpenGL::GL)
target_link_libraries(nes-emu glfw)
target_link_libraries(nes-emu GLEW::GLEW)


file(COPY "lib/glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})
