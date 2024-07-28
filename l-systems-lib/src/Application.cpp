#include <stdexcept>
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
    running = true;
    setup_components();
    window.loop([&]() {
        GL_CHECK_ERROR();
        for (auto &[component, _]: components) {
            component->draw();
        }
    });
}

void Application::add_component(std::shared_ptr<Drawable> drawable, std::function<Viewport(Application &)> viewport_function) {
    if (running) {
        throw std::runtime_error("Cannot add component while application is running");
    }
    components.emplace_back(std::move(drawable), std::move(viewport_function));
}

void Application::setup_components() {
    for (auto &[component, _]: components) {
        component->init();
    }

    window.set_window_resize_callback([&](int, int) {
        for (auto &[component, get_viewport]: components) {
            component->update_viewport(get_viewport(*this));
        }
    });

    window.set_mouse_button_callback([this](int button, int action, int mods, double x, double y) {
        for (auto &[component, _]: components) {
            component->on_mouse_button(button, action, mods, x, y);
        }
    });

    window.set_cursor_position_callback([this](double x, double y) {
        for (auto &[component, _]: components) {
            component->on_cursor_position(x, y);
        }
    });
}
