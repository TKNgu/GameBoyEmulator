#Cmake version.
cmake_minimum_required(VERSION 3.10.2)
include(ExternalProject)
set(CMAKE_EXPORT_COMPINLE_COMMANDS ON)
set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
set(CMAKE_BUILD_TYPE debug)

# AIAPI
project(NguBoy VERSION 0.1.0 LANGUAGES C DESCRIPTION "Ngu Game Boy Emulator")
if(BUILD_DEBUG)
    add_compile_definitions(DEBUG)
endif(BUILD_DEBUG)

include_directories(src/)

file(GLOB SRC src/*.c)

add_executable(${CMAKE_PROJECT_NAME} ${SRC})
target_link_libraries(${CMAKE_PROJECT_NAME} SDL2 SDL2_image m)
add_custom_target(run ${CMAKE_PROJECT_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
