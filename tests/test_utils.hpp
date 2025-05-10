#include <gtest/gtest.h>
#include "Application.hpp"
#include <glm/glm.hpp>
#include "test_properties.hpp"
#include "GLFW/glfw3.h"
#include "grammar.h"

class TestDrawable : public Drawable {
public:
    TestDrawable(
            const Viewport &viewport,
            const std::shared_ptr<View> &view,
            Application &application,
            const std::function<void()> &_init_callback,
            const std::function<void()> &_task_callback
    ) :
            Drawable(viewport, view),
            application(application),
            init_callback(_init_callback),
            task_callback(_task_callback) {}

    void init() override {
        init_callback();
    }

    void draw() override {
        try {
            task_callback();
        } catch (const std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
        glfwSetWindowShouldClose(application.get_window().get_window(), GLFW_TRUE);
    }

private:
    Application &application;
    const std::function<void()> &init_callback;
    const std::function<void()> &task_callback;
};

class TextContext {
public:
    TextContext(const std::function<void()> &init, const std::function<void()> &task) : application(
            400,
            300,
            "Unit Test Application",
            glm::vec3(0.0f, 0.0f, 0.0f),
            3,
            2
    ) {
        auto viewport_function = [](Application &application) -> Viewport {
            return Viewport{
                    .left = 0,
                    .top = 0,
                    .width = application.get_window().get_width(),
                    .height = application.get_window().get_height(),
                    .window_width = application.get_window().get_width(),
                    .window_height = application.get_window().get_height()
            };
        };

        auto rotation_view = std::make_shared<RotateView>(
                RotateView::Direction::COUNTER_CLOCKWISE,
                1.0,
                1.0,
                1.0
        );

        auto test_component = std::make_shared<TestDrawable>(
                viewport_function(application),
                rotation_view,
                application,
                init,
                task
        );

        application.add_component(test_component, viewport_function);
        application.run();
    }

    Application application;
};

void run_opengl_test(
        const std::function<void()> &init,
        const std::function<void()> &task
);
