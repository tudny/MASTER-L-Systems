#include <iostream>
#include "debug.hpp"

void LOG(const char *message) {
    std::cout << message << std::endl;
}

void LOG(const std::string &message) {
    std::cout << message << std::endl;
}
