#ifndef LSYSTEMS_DEBUG_HPP
#define LSYSTEMS_DEBUG_HPP

#include <string>
#include "glm/ext/matrix_float4x4.hpp"

void LOG(const char *message);

void LOG(const std::string &message);

void print_mat4(glm::mat4 matrix);

#endif //LSYSTEMS_DEBUG_HPP
