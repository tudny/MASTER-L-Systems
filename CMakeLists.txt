cmake_minimum_required(VERSION 3.22)
project(LSystems)

# Default to Release build if not specified
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# Compilation setup
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Setup OpenGL dependencies
add_definitions(-DGLEW_STATIC)
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)

# Doxygen setup
find_package(Doxygen)
if (DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile @ONLY)
    add_custom_target(doc
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
    )
endif (DOXYGEN_FOUND)

# Add local submodules
SET(
        LOCAL_SUBMODULES
        "l-systems-grammar-lib"
        "l-systems-lib"
        "l-systems"
)
SET(
        ALL_SOURCES
        ""
)
FOREACH (submodule ${LOCAL_SUBMODULES})
    message(STATUS "Adding submodule: ${submodule}")
    add_subdirectory(${submodule})
    list(APPEND ALL_SOURCES ${submodule}/src)
    # If ends with _lib, add to include directories
    STRING(REGEX MATCH ".*lib$" is_lib ${submodule})
    IF (is_lib)
        message(STATUS "  Detected library: ${submodule}")
        include_directories(${PROJECT_SOURCE_DIR}/${submodule}/src)
    ENDIF ()
ENDFOREACH ()

# Add GTest
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
endif ()

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(TEST_EXEC_NAME all_tests_exec)
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS ${TEST_PATH}/*.cpp ${TEST_PATH}/*.hpp)
list(FILTER ALL_SOURCES EXCLUDE REGEX ".*main.cpp$")

add_executable(
        ${TEST_EXEC_NAME}
        ${TEST_FILES}
        ${ALL_SOURCES}
)
target_link_libraries(
        ${TEST_EXEC_NAME}
        GTest::gtest_main
        GTest::gmock_main
        L-Systems-lib
        L-Systems-grammar-lib
)

include(GoogleTest)
gtest_discover_tests(${TEST_EXEC_NAME})
