project(L-Systems-lib)

set(
        LIB_NAME
        L-Systems-lib
)

set(
        LIBRARY_SOURCES
        src/lib.cpp
        src/lib.hpp
        src/Application.cpp
        src/Application.hpp
        src/GLFWWindowWrapper.cpp
        src/GLFWWindowWrapper.hpp
        src/errors.cpp
        src/errors.hpp
        src/Drawable.cpp
        src/Drawable.hpp
        src/Viewport.cpp
        src/Viewport.hpp
        src/shader.cpp
        src/shader.hpp
        src/debug.cpp
        src/debug.hpp
)

add_library(
        ${LIB_NAME}
        ${LIBRARY_SOURCES}
)

target_link_libraries(
        ${LIB_NAME}
        PUBLIC
        PRIVATE glfw
        PRIVATE libglew_static
        PRIVATE glm
)
