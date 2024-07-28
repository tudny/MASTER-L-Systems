#include <iostream>
#include "lib.hpp"
#include "Application.hpp"
#include "constants.hpp"
#include "GL/glew.h"
#include "shader.hpp"
#include "assets.hpp"

class SimpleDrawable : public Drawable {
public:

    explicit SimpleDrawable(const Viewport &viewport) : Drawable(viewport) {}

    ~SimpleDrawable() override = default;

    void init() override {
        if (!simple_shader_program) {
            simple_shader_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("simple.vert"), GL_VERTEX_SHADER},
                    Shader{SHADER_PATH("simple.frag"), GL_FRAGMENT_SHADER}
            });
        }
        this->shader_program = simple_shader_program;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo_point);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_point);
        shader_program->setAttribute("position", 3, sizeof(glm::vec3), 0);

        float vertices[] = {
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.0f, 0.5f, 0.0f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        shader_program->setAttribute("color", 3, sizeof(glm::vec3), 0);

        float colors[] = {
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
    }

    void draw() override {
        auto view_matrix = viewport.local_fixed_ratio_to_standard_square();

        this->shader_program->use();
        this->shader_program->setUniform("view_matrix", view_matrix);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        this->shader_program->unuse();
    }

private:
    GLuint vao = 0;
    GLuint vbo_point = 0;
    GLuint vbo_color = 0;

    static std::shared_ptr<ShaderProgram> simple_shader_program;
};

std::shared_ptr<ShaderProgram> SimpleDrawable::simple_shader_program;

void add_simple_component(Application &application) {

    auto viewport_function = [](Application &application) -> Viewport {
        return Viewport{
                .left = 0,
                .top = 0,
                .width = application.get_window().get_width() / 2,
                .height = application.get_window().get_height() / 2,
                .window_width = application.get_window().get_width(),
                .window_height = application.get_window().get_height()
        };
    };

    application.add_component(
            std::make_shared<SimpleDrawable>(viewport_function(application)),
            viewport_function
    );
}

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
        add_simple_component(application);
        application.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
