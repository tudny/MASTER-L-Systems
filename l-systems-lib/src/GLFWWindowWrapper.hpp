/** @file
 * @brief GLFW window wrapper declaration
 *
 * This file contains the declaration of the GLFW window wrapper class.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 07.07.2024
*/

#ifndef LSYSTEMS_LIB_GLFWWINDOWWRAPPER_HPP
#define LSYSTEMS_LIB_GLFWWINDOWWRAPPER_HPP

#include <string_view>
#include <functional>
#include "glm/glm.hpp"

struct GLFWwindow;

/// GLFW window wrapper class
class GLFWWindowWrapper {
public:

    /**
     * @brief GLFW window wrapper constructor
     *
     * GLFW window wrapper constructor creates a window with given parameters.
     *
     * @param width Window width
     * @param height Window height
     * @param title Window title
     * @param clear_color Clear color
     * @param opengl_major_version OpenGL major version
     * @param opengl_minor_version OpenGL minor version
     */
    GLFWWindowWrapper(
            int width,
            int height,
            std::string_view title,
            glm::vec3 clear_color = glm::vec3(0.0f, 0.0f, 0.0f),
            int opengl_major_version = 3,
            int opengl_minor_version = 2
    );

    /**
     * @brief GLFW window wrapper destructor
     *
     * GLFW window wrapper destructor is a default destructor.
     */
    ~GLFWWindowWrapper();

    /**
     * @brief Loop
     *
     * Loop the application with given draw callback.
     *
     * @param draw_callback Draw callback
     */
    void loop(std::function<void()> const &draw_callback);

    /**
     * @brief Get the window
     *
     * Get the window.
     *
     * @return GLFW window
     */
    GLFWwindow *get_window();

    /**
     * @brief Set the window resize callback
     *
     * Set the window resize callback.
     *
     * @param callback Callback
     */
    void set_window_resize_callback(std::function<void(int, int)> const &callback);

    /**
     * @brief Set the cursor position callback
     *
     * Set the cursor position callback.
     *
     * @param callback Callback
     */
    void set_cursor_position_callback(std::function<void(double, double)> const &callback);

    /**
     * @brief Set the mouse button callback
     *
     * Set the mouse button callback.
     *
     * @param callback Callback
     */
    void set_mouse_button_callback(std::function<void(int, int, int, double, double)> const &callback);

    /**
     * @brief Get the width
     *
     * Get the width of the window.
     *
     * @return Window width
     */
    [[nodiscard]] int get_width() const;

    /**
     * @brief Get the height
     *
     * Get the height of the window.
     *
     * @return Window height
     */
    [[nodiscard]] int get_height() const;

private:
    /// GLFW window
    GLFWwindow *window;
    /// Window width
    int width;
    /// Window height
    int height;
    /// Clear color
    glm::vec3 clear_color;

    std::function<void(int, int)> window_resize_callback;
    std::function<void(double, double)> cursor_position_callback;
    std::function<void(int, int, int, double, double)> mouse_button_callback;

    /**
     * @brief Detect window resize
     *
     * Detect window resize.
     */
    void detect_window_resize();

    /**
     * @brief Detect cursor position
     *
     * Detect cursor position.
     */
    static std::unordered_map<GLFWwindow *, GLFWWindowWrapper *> window_wrapper_map;

    /**
     * @brief GLFW window resize callback
     *
     * This function is called when the window is resized in GLFW.
     *
     * @param window Window
     */
    static void glfw_cursor_position_callback(GLFWwindow *window, double x, double y);

    /**
     * @brief GLFW mouse button callback
     *
     * This function is called when the mouse button is pressed in GLFW.
     *
     * @param window Window
     * @param button Button
     * @param action Action
     * @param mods Mods
     */
    static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
};

#endif //LSYSTEMS_LIB_GLFWWINDOWWRAPPER_HPP
