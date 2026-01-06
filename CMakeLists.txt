# aria_shell - Aria Shell (AriaSH)
# Interactive REPL and scripting shell for Aria language
#
# ARIA-021: Shell Job Control State Machine
#
# Build:
#   mkdir build && cd build
#   cmake ..
#   make

cmake_minimum_required(VERSION 3.16)
project(aria_shell
    VERSION 0.1.0
    DESCRIPTION "Interactive Shell for Aria Language"
    LANGUAGES CXX
)

# Require C++17 for std::optional, std::variant, etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Threading support
find_package(Threads REQUIRED)

# -----------------------------------------------------------------------------
# Library: aria_shell_job (Job Control - ARIA-021)
# -----------------------------------------------------------------------------
add_library(aria_shell_job STATIC
    src/job/job_state.cpp
    src/job/job_control.cpp
    src/job/stream_controller.cpp
)

target_include_directories(aria_shell_job
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(aria_shell_job PUBLIC Threads::Threads)

set_target_properties(aria_shell_job PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# Main executable: ariash
# -----------------------------------------------------------------------------
add_executable(ariash
    src/repl/main.cpp
)

target_link_libraries(ariash PRIVATE aria_shell_job)

set_target_properties(ariash PROPERTIES
    OUTPUT_NAME "ariash"
)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
option(BUILD_TESTS "Build test executable" ON)

if(BUILD_TESTS)
    enable_testing()

    # Job control tests would go here
    # add_executable(test_job_control tests/test_job_control.cpp)
    # target_link_libraries(test_job_control PRIVATE aria_shell_job)
    # add_test(NAME job_control_tests COMMAND test_job_control)
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS aria_shell_job
    EXPORT aria_shellTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY inc/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "aria_shell ${PROJECT_VERSION} - Aria Interactive Shell")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "")
