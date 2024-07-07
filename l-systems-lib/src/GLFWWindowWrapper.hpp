#ifndef LSYSTEMS_LIB_GLFWWINDOWWRAPPER_HPP
#define LSYSTEMS_LIB_GLFWWINDOWWRAPPER_HPP

#include <string_view>
#include <functional>
#include "glm/glm.hpp"

struct GLFWwindow;

class GLFWWindowWrapper {
public:
    GLFWWindowWrapper(
            int width,
            int height,
            std::string_view title,
            glm::vec3 clear_color = glm::vec3(0.0f, 0.0f, 0.0f),
            int opengl_major_version = 3,
            int opengl_minor_version = 2
    );

    ~GLFWWindowWrapper();

    void loop(std::function<void()> const &draw_callback);

    GLFWwindow *get_window();

    void set_window_resize_callback(std::function<void(int, int)> const &callback);

    void set_cursor_position_callback(std::function<void(double, double)> const &callback);

    void set_mouse_button_callback(std::function<void(int, int, int, double, double)> const &callback);

    [[nodiscard]] int get_width() const;

    [[nodiscard]] int get_height() const;

private:
    GLFWwindow *window;
    int width;
    int height;
    glm::vec3 clear_color;
    std::function<void(int, int)> window_resize_callback;
    std::function<void(double, double)> cursor_position_callback;
    std::function<void(int, int, int, double, double)> mouse_button_callback;

    void detect_window_resize();

    static std::unordered_map<GLFWwindow *, GLFWWindowWrapper *> window_wrapper_map;

    static void glfw_cursor_position_callback(GLFWwindow *window, double x, double y);

    static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
};

#endif //LSYSTEMS_LIB_GLFWWINDOWWRAPPER_HPP
