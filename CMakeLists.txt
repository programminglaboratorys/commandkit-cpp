cmake_minimum_required(VERSION 3.15)

# The project name is written here and nowhere else in the CMake files.
project(commandkit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# uppercase copy of the name, used as the prefix of the option names below
string(TOUPPER "${PROJECT_NAME}" PROJECT_PREFIX)

# build tests/examples only when this is the top-level project, so it can be
# dropped into another project with add_subdirectory without side effects.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IS_TOP_LEVEL ON)
else()
    set(IS_TOP_LEVEL OFF)
endif()

option(${PROJECT_PREFIX}_BUILD_TESTS    "Build ${PROJECT_NAME} tests"    ${IS_TOP_LEVEL})
option(${PROJECT_PREFIX}_BUILD_EXAMPLES "Build ${PROJECT_NAME} examples" ${IS_TOP_LEVEL})

add_library(${PROJECT_NAME} STATIC
    src/library.cpp
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# tests / examples
if(${PROJECT_PREFIX}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(${PROJECT_PREFIX}_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# install
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
