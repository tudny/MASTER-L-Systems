/** @file
 * @brief Errors functions declaration
 *
 * This file contains the declaration of the functions for error checking.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 07.07.2024
*/

#ifndef LSYSTEMS_LIB_ERRORS_HPP
#define LSYSTEMS_LIB_ERRORS_HPP


/**
 * @brief Check OpenGL error
 *
 * Check the OpenGL error and print the error message.
 *
 * @param file File name
 * @param line Line number
 */
void gl_check_error(const char *file, int line);

/**
 * @brief Check OpenGL error
 *
 * Macro for checking the OpenGL error.
 * It calls the gl_check_error function with the current file and line.
 */
#define GL_CHECK_ERROR() gl_check_error(__FILE__, __LINE__)


#endif //LSYSTEMS_LIB_ERRORS_HPP
