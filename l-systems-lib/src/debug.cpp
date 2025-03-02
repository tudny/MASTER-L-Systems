#include <iostream>
#include "debug.hpp"

void LOG(const char *message) {
    std::cout << message << std::endl;
}

void LOG(const std::string &message) {
    std::cout << message << std::endl;
}

void print_mat4(glm::mat4 matrix) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
