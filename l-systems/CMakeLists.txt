project(L-Systems)

set(
        TARGET_NAME
        L-Systems
)

set(
        EXECUTABLE_SOURCES
        src/main.cpp
        src/constants.hpp
        src/cube.cpp
        src/cube.hpp
        src/baseline.cpp
        src/baseline.h
)

add_executable(
        ${TARGET_NAME}
        ${EXECUTABLE_SOURCES}
)

target_link_libraries(
        ${TARGET_NAME}
        PRIVATE
        L-Systems-lib
        L-Systems-grammar
        PRIVATE glfw
        PRIVATE libglew_static
        PRIVATE glm
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/properties.hpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/src/properties.hpp
)
include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
)
