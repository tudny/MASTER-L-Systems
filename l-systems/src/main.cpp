#include <iostream>
#include "lib.hpp"
#include "Application.hpp"
#include "constants.hpp"

int main() {
    std::cout << greeter() << std::endl;

    try {
        Application application{
                DEFAULT_WINDOW_WIDTH,
                DEFAULT_WINDOW_HEIGHT,
                DEFAULT_WINDOW_TITLE,
                DEFAULT_CLEAR_COLOR,
                OPENGL_MAJOR_VERSION,
                OPENGL_MINOR_VERSION
        };
        application.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
