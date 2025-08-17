/** @file
 * @brief Constants for the application
 *
 * This file contains the constants for the application.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 07.07.2024
*/

#ifndef LSYSTEMS_CONSTANTS_HPP
#define LSYSTEMS_CONSTANTS_HPP

#include <string>

/// Default window width
constexpr int DEFAULT_WINDOW_WIDTH = 800;
/// Default window height
constexpr int DEFAULT_WINDOW_HEIGHT = 600;
/// Default window title
constexpr std::string_view DEFAULT_WINDOW_TITLE = "L-Systems";
/// Default clear color is black
constexpr glm::vec3 DEFAULT_CLEAR_COLOR = glm::vec3(1.0f, 1.0f, 1.0f);

/// OpenGL version major
constexpr int OPENGL_MAJOR_VERSION = 3;
/// OpenGL version minor
constexpr int OPENGL_MINOR_VERSION = 2;

constexpr int REQUIRED_NUMBER_OF_BUFFERS = 16;

#endif //LSYSTEMS_CONSTANTS_HPP
