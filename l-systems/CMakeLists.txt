project(L-Systems)

set(
        TARGET_NAME
        L-Systems
)

set(
        EXECUTABLE_SOURCES
        src/main.cpp
        src/constants.hpp
)

add_executable(
        ${TARGET_NAME}
        ${EXECUTABLE_SOURCES}
)

target_link_libraries(
        ${TARGET_NAME}
        PRIVATE
        L-Systems-lib
        PRIVATE glfw
        PRIVATE libglew_static
        PRIVATE glm
)
