cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Helper Function - Add Test with All Standard Setup
# ============================================================================

function(add_chess_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE chess-engine)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_23)

    if(NOT MSVC)
        target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 60)
endfunction()

# ============================================================================
# Add Tests
# ============================================================================

# Board Tests
add_chess_test(BoardTests board_tests.cpp)

# Search Tests
add_chess_test(SearchTests search_tests.cpp)
