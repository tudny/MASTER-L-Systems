/** @file
 * @brief Greeter function declaration
 *
 * This file contains the declaration of the greeter function.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 29.06.2024
*/
#include <string>
#include "shader.hpp"
#include "memory"

/**
 * @brief Greeter function
 *
 * Greeter function returns a string "Hello, World!"
 *
 * @return std::string "Hello, World!"
 */
std::string greeter();

// Exported for testing purposes
void global_run_prefix_sum(std::shared_ptr<ShaderProgram> &shader, GLuint ssbo, size_t size);

