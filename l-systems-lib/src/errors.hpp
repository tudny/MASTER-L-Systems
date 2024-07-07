#ifndef LSYSTEMS_LIB_ERRORS_HPP
#define LSYSTEMS_LIB_ERRORS_HPP


void gl_check_error(const char *file, int line);

#define GL_CHECK_ERROR() gl_check_error(__FILE__, __LINE__)


#endif //LSYSTEMS_LIB_ERRORS_HPP
