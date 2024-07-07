#ifndef LSYSTEMS_LIB_APPLICATION_HPP
#define LSYSTEMS_LIB_APPLICATION_HPP


#include <memory>
#include "GLFWWindowWrapper.hpp"
#include "Drawable.hpp"

class Application {
public:
    Application(int width, int height, std::string_view title, glm::vec3 clear_color, int opengl_major_version, int opengl_minor_version);
    void run();

private:
    GLFWWindowWrapper window;

    using component_t = std::pair<std::shared_ptr<Drawable>, std::function<Viewport()>>;
};


#endif //LSYSTEMS_LIB_APPLICATION_HPP
