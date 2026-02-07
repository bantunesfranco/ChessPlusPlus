cmake_minimum_required(VERSION 3.20)

project(chess-engine
        VERSION 1.0.0
        DESCRIPTION "A high-performance chess engine library in C++"
        LANGUAGES CXX
)

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Flags
if(MSVC)
    # Visual Studio
    add_compile_options(/W4 /WX)  # Warnings as errors
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " /Zi /Od")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /O2")
else()
    # GCC/Clang
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-Wno-deprecated-declarations)  # Optional: suppress deprecations

    # Debug flags
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -g -O0")

    # Release optimization
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -march=native")
endif()

# ============================================================================
# Library Target
# ============================================================================

# Collect all source files (but NOT headers)
file(GLOB_RECURSE CHESS_SOURCES
        "src/internal/*.cpp"
        "src/*.cpp"
)

file(GLOB_RECURSE CHESS_HEADERS
        "include/internal/*.hpp"
        "include/*.hpp"
)

# Create library
add_library(chess-engine ${CHESS_SOURCES} ${CHESS_HEADERS})

# Include directories
target_include_directories(chess-engine
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# C++ Standard for library
target_compile_features(chess-engine PUBLIC cxx_std_23)

# ============================================================================
# Example/Demo Executable
# ============================================================================

# Only build demo if explicitly requested or if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(chess-demo chess.cpp)

    target_link_libraries(chess-demo PRIVATE chess-engine)

    # Demo binary in output directory
    set_target_properties(chess-demo PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# ============================================================================
# Installation Targets
# ============================================================================

# Install library
install(TARGETS chess-engine
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

# Install public headers
install(DIRECTORY include/chess
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
)

# Install CMake config files (for find_package)
install(FILES cmake/chess-engine-config.cmake
        DESTINATION lib/cmake/chess-engine
        OPTIONAL
)

# ============================================================================
# Optional: Unit Tests
# ============================================================================

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Optional: Documentation
# ============================================================================

option(BUILD_DOCS "Build documentation" OFF)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
        )
        add_custom_target(docs ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
        )
    endif()
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "Chess Engine Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Docs: ${BUILD_DOCS}")