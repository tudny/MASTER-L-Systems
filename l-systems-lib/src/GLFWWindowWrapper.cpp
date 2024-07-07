#include <stdexcept>
#include "GLFWWindowWrapper.hpp"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

std::unordered_map<GLFWwindow *, GLFWWindowWrapper *> GLFWWindowWrapper::window_wrapper_map;

GLFWWindowWrapper::GLFWWindowWrapper(
        int width,
        int height,
        std::string_view title,
        glm::vec3 clear_color,
        int opengl_major_version,
        int opengl_minor_version
) : width(width), height(height), clear_color(clear_color) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opengl_major_version);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opengl_minor_version);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error(
                "Failed to initialize GLEW -- " +
                std::string(reinterpret_cast<const char *>(glewGetErrorString(err)))
        );
    }

    window_wrapper_map[window] = this;

    glfwSetCursorPosCallback(window, glfw_cursor_position_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
}

GLFWWindowWrapper::~GLFWWindowWrapper() {
    window_wrapper_map.erase(window);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GLFWWindowWrapper::loop(std::function<void()> const &draw_callback) {
    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        detect_window_resize();
        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_callback();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

GLFWwindow *GLFWWindowWrapper::get_window() {
    return window;
}

void GLFWWindowWrapper::set_window_resize_callback(const std::function<void(int, int)> &callback) {
    window_resize_callback = callback;
}

void GLFWWindowWrapper::set_cursor_position_callback(const std::function<void(double, double)> &callback) {
    cursor_position_callback = callback;
}

void GLFWWindowWrapper::set_mouse_button_callback(const std::function<void(int, int, int, double, double)> &callback) {
    mouse_button_callback = callback;
}

int GLFWWindowWrapper::get_width() const {
    return width;
}

int GLFWWindowWrapper::get_height() const {
    return height;
}

void GLFWWindowWrapper::detect_window_resize() {
    int new_width, new_height;
    glfwGetWindowSize(window, &new_width, &new_height);

    if (new_width != width || new_height != height) {
        width = new_width;
        height = new_height;
        glViewport(0, 0, width, height);
        if (window_resize_callback) {
            window_resize_callback(width, height);
        }
    }
}

void GLFWWindowWrapper::glfw_cursor_position_callback(GLFWwindow *window, double x, double y) {
    auto it = window_wrapper_map.find(window);
    if (it != window_wrapper_map.end() && it->second->cursor_position_callback) {
        it->second->cursor_position_callback(x, y);
    }
}

void GLFWWindowWrapper::glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    auto it = window_wrapper_map.find(window);
    if (it != window_wrapper_map.end() && it->second->mouse_button_callback) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        it->second->mouse_button_callback(button, action, mods, x, y);
    }
}


