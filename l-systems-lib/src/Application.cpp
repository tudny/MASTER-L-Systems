#include "Application.hpp"
#include "errors.hpp"

Application::Application(
        int width,
        int height,
        std::string_view title,
        glm::vec3 clear_color,
        int opengl_major_version,
        int opengl_minor_version
) : window(width, height, title, clear_color, opengl_major_version, opengl_minor_version) {
    GL_CHECK_ERROR();
}

void Application::run() {
    window.loop([]() {
        GL_CHECK_ERROR();
    });
}
