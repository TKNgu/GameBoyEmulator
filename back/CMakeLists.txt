cmake_minimum_required(VERSION 3.10.2)

project(GeameBoyEmulator VERSION 1.0.0 LANGUAGES CXX DESCRIPTION "Game Boy Emulator")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Build type: DEBUG")
    add_compile_definitions(DEBUG)
    if(MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic -g3 -O0 -fno-omit-frame-pointer)
    endif()
else()
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    if(NOT MSVC)
        add_compile_options(-O2)
    endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/lib/local/include)
link_directories(
    ${CMAKE_SOURCE_DIR}/lib/local/lib
    ${CMAKE_SOURCE_DIR}/lib/local/lib64
)

file(GLOB_RECURSE SRC
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

add_executable(${PROJECT_NAME} ${SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-no-pie")

target_link_libraries(${PROJECT_NAME})

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env
        LD_LIBRARY_PATH=${CMAKE_SOURCE_DIR}/lib/local/lib/
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
