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

project(NguGB VERSION 1.0.0 LANGUAGES C DESCRIPTION "Ngu GB Emulator")
if(BUILD_DEBUG)
    add_compile_definitions(DEBUG)
endif(BUILD_DEBUG)

if (BUILD_LIB)
endif()

include_directories()

link_directories()

file(GLOB_RECURSE SRC src/*.c)

add_executable(${CMAKE_PROJECT_NAME} ${SRC})
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY C_STANDARD 23)
target_link_libraries(${CMAKE_PROJECT_NAME})
add_custom_target(run ${CMAKE_COMMAND} -E env "LD_LIBRARY_PATH=${CMAKE_SOURCE_DIR}/lib/local/lib/"
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} data/cpu_instrs.gb
  DEPENDS ${CMAKE_PROJECT_NAME}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
