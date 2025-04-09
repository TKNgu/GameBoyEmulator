cmake_minimum_required(VERSION 3.10.2)
include(ExternalProject)
set(CMAKE_EXPORT_COMPINLE_COMMANDS ON)
set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
set(CMAKE_BUILD_TYPE debug)
if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

project(GameBoyEmulator VERSION 1.0.0 LANGUAGES C DESCRIPTION "Gameboy Emulator")
if(BUILD_DEBUG)
    add_compile_definitions(DEBUG)
endif(BUILD_DEBUG)

if (BUILD_LIB)

ExternalProject_Add(libSDL2
GIT_REPOSITORY git@github.com:libsdl-org/SDL.git
GIT_TAG release-2.30.8
SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/SDL2
CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_SOURCE_DIR}/lib/local)

ExternalProject_Add(libtinycc
    GIT_REPOSITORY git://repo.or.cz/tinycc.git
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/tinycc
    CONFIGURE_COMMAND ${CMAKE_SOURCE_DIR}/lib/tinycc/configure
        --prefix="${CMAKE_SOURCE_DIR}/lib/local"  
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install)

endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/local/include/SDL2/
    ${CMAKE_SOURCE_DIR}/lib/local/include/
    ${CMAKE_SOURCE_DIR}/src/
)

link_directories()

file(GLOB SRC src/*.c src/cpu/*.c)

add_executable(${CMAKE_PROJECT_NAME} ${SRC})
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY C_STANDARD 23)
target_link_libraries(${CMAKE_PROJECT_NAME} SDL2
  ${CMAKE_SOURCE_DIR}/lib/local/lib/libtcc.a)
add_custom_target(run 
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} data/Zelda.gbc
  DEPENDS ${CMAKE_PROJECT_NAME} 
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_custom_target(test 
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} data/cpu_instrs.gb
  DEPENDS ${CMAKE_PROJECT_NAME} 
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(RunTime ${PROJECT_SOURCE_DIR}/sample/runtime.c)
target_link_libraries(RunTime 
  ${CMAKE_SOURCE_DIR}/lib/local/lib/libtcc.a)
add_custom_target(run_RunTime
  RunTime
  DEPENDS RunTime
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
