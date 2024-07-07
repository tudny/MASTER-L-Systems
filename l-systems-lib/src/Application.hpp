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

private:
    /// Window object
    GLFWWindowWrapper window;

    /// Component type
    using component_t = std::pair<std::shared_ptr<Drawable>, std::function<Viewport()>>;
};


#endif //LSYSTEMS_LIB_APPLICATION_HPP
