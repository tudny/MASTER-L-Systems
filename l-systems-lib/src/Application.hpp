/** @file
 * @brief Application class declaration
 *
 * This file contains the declaration of the Application class.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 07.07.2024
*/
#ifndef LSYSTEMS_LIB_APPLICATION_HPP
#define LSYSTEMS_LIB_APPLICATION_HPP


#include <memory>
#include "GLFWWindowWrapper.hpp"
#include "Drawable.hpp"
#include <GLFW/glfw3.h>

class KeyState {
public:

    enum KeyWhenAction {
        ON_CLICK,
        WHEN_PRESSED
    };

    KeyState(
            int key,
            const std::function<void()> &keyAction,
            KeyWhenAction keyWhenAction
    ) :
            key(key),
            key_action(keyAction),
            key_when_action(keyWhenAction) {}

    void update_key_actions(int _key, int action) {
        bool was_changed = false;
        if (key == _key) {
            if (action == GLFW_PRESS) {
                was_changed = true;
                is_pressed = true;
            } else if (action == GLFW_RELEASE) {
                was_changed = true;
                is_pressed = false;
            }
        }

        if (is_pressed) {
            if (key_when_action == WHEN_PRESSED || (key_when_action == ON_CLICK && was_changed)) {
                key_action();
            }
        }
    }

private:
    int key{};
    std::function<void()> key_action;
    KeyWhenAction key_when_action;

    bool is_pressed = false;
};

/// Application class abstracts the application
class Application {
public:

    /**
     * @brief Application constructor
     *
     * Application constructor creates a window with given parameters.
     *
     * @param width Window width
     * @param height Window height
     * @param title Window title
     * @param clear_color Clear color
     * @param opengl_major_version OpenGL major version
     * @param opengl_minor_version OpenGL minor version
     */
    Application(
            int width,
            int height,
            std::string_view title,
            glm::vec3 clear_color,
            int opengl_major_version,
            int opengl_minor_version
    );

    /**
     * @brief Run the application
     *
     * Run the application and start the main loop.
     */
    void run();

    /**
     * @brief Add component to the application
     *
     * Add a component to the application.
     *
     * @param drawable Drawable object
     * @param viewport_function Viewport function
     */
    void add_component(std::shared_ptr<Drawable> drawable, std::function<Viewport(Application &)> viewport_function);

    /**
     * @brief Get window
     *
     * Get the window object.
     *
     * @return Window object
     */
    GLFWWindowWrapper &get_window() {
        return window;
    }

    /**
     * @brief Register on key click
     *
     * Register on key click callback.
     * Callback is called when the key is pressed.
     *
     * @param key Key
     * @param keyAction Key action callback
     * */
    void register_on_key_click(int key, const std::function<void()> &keyAction);

    /**
     * @brief Register on key pressed
     *
     * Register on key pressed callback.
     * Callback is called always when the key is pressed.
     *
     * @param key Key
     * @param keyAction Key action callback
     * */
    void register_on_key_pressed(int key, const std::function<void()> &keyAction);

private:

    /// Setup components
    void setup_components();

    /// Window object
    GLFWWindowWrapper window;

    /// Is the application running
    bool running = false;

    /// Component type
    using component_t = std::pair<std::shared_ptr<Drawable>, std::function<Viewport(Application &)>>;

    /// Components
    std::vector<component_t> components;

    /// Key states
    std::vector<KeyState> key_states;
};


#endif //LSYSTEMS_LIB_APPLICATION_HPP
