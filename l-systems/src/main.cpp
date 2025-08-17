#include <iostream>
#include "lib.hpp"
#include "Application.hpp"
#include "constants.hpp"
#include "properties.hpp"
#include "system.hpp"
#include "args.hpp"

int main(int argc, char *argv[]) {
    std::cout << greeter() << std::endl;

    auto context = parse_args(argc, argv);

    try {
        Application application{
                DEFAULT_WINDOW_WIDTH,
                DEFAULT_WINDOW_HEIGHT,
                DEFAULT_WINDOW_TITLE,
                DEFAULT_CLEAR_COLOR,
                OPENGL_MAJOR_VERSION,
                OPENGL_MINOR_VERSION
        };
        register_system(application, context);
        check_buffers_compatibility(REQUIRED_NUMBER_OF_BUFFERS);
        application.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
