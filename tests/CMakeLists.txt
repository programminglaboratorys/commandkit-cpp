# Test runner. No framework: each check prints a message and the exit code
# tells ctest whether everything passed. Swap in Catch2/doctest if you want one.
add_executable(${PROJECT_NAME}_tests test_main.cpp)
target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME})
if(MSVC)
    target_compile_options(${PROJECT_NAME}_tests PRIVATE /W4 /utf-8)
else()
    target_compile_options(${PROJECT_NAME}_tests PRIVATE -Wall -Wextra -pedantic)
endif()
add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
