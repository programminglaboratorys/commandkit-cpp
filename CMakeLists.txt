cmake_minimum_required(VERSION 3.14)

project(commandkit VERSION 0.1.0 LANGUAGES CXX)

string(TOUPPER "${PROJECT_NAME}" PROJECT_PREFIX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IS_TOP_LEVEL ON)
else()
    set(IS_TOP_LEVEL OFF)
endif()

option(${PROJECT_PREFIX}_BUILD_TESTS    "Build ${PROJECT_NAME} tests"    ${IS_TOP_LEVEL})
option(${PROJECT_PREFIX}_BUILD_EXAMPLES "Build ${PROJECT_NAME} examples" ${IS_TOP_LEVEL})
option(CMDKIT_USE_HASHMAP "Use std::unordered_map instead of sorted vector" OFF)
option(CMDKIT_USE_ABSEIL  "Use absl::flat_hash_map (requires Abseil)"       OFF)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

if(CMDKIT_USE_HASHMAP)
    target_compile_definitions(${PROJECT_NAME} INTERFACE CMDKIT_USE_HASHMAP)
endif()
if(CMDKIT_USE_ABSEIL)
    find_package(absl REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE absl::flat_hash_map)
    target_compile_definitions(${PROJECT_NAME} INTERFACE CMDKIT_USE_ABSEIL)
endif()

if(${PROJECT_PREFIX}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(${PROJECT_PREFIX}_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)