cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test Executable: Board Tests
# ============================================================================

add_executable(board_tests
        board_tests.cpp
)

# Link against main chess library
target_link_libraries(board_tests PRIVATE chess-engine)

# Include paths
target_include_directories(board_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# C++ Standard
target_compile_features(board_tests PRIVATE cxx_std_23)

# Compiler flags for tests
if(NOT MSVC)
    target_compile_options(board_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Register Tests with CTest
# ============================================================================

# Add test to CTest
add_test(
        NAME BoardTests
        COMMAND board_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Set test properties
set_tests_properties(BoardTests
        PROPERTIES
        TIMEOUT 30  # 30 second timeout
        LABELS "board"
)

# ============================================================================
# Optional: Additional Tests (Add as needed)
# ============================================================================

# Example: Add more test executables here
# add_executable(move_generation_tests move_generation_tests.cpp)
# target_link_libraries(move_generation_tests PRIVATE chess-engine)
# add_test(NAME MoveGenerationTests COMMAND move_generation_tests)

# ============================================================================
# Test Output
# ============================================================================

# Print test summary
message(STATUS "Tests Configuration:")
message(STATUS "  Board Tests: Enabled")
message(STATUS "  Test Binary: ${CMAKE_BINARY_DIR}/board_tests")
message(STATUS "  Run tests with: ctest --output-on-failure")