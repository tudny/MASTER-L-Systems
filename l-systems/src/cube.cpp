#include "cube.hpp"
#include "assets.hpp"
#include "errors.hpp"

class CubeDrawable : public Drawable {
public:

    explicit CubeDrawable(const Viewport &viewport) : Drawable(viewport) {}

    ~CubeDrawable() override = default;

    void init() override {
        preload_shader_program();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo_point);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_point);
        shader_program->setAttribute("position", 3, 0, 0);

        float vertices[] = {
                -0.5f, -0.5f, 0.5f,
                0.5f, -0.5f, 0.5f,
                0.5f, 0.5f, 0.5f,
                -0.5f, 0.5f, 0.5f,
                -0.5f, -0.5f, -0.5f,
                0.5f, -0.5f, -0.5f,
                0.5f, 0.5f, -0.5f,
                -0.5f, 0.5f, -0.5f,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        shader_program->setAttribute("color", 3, 0, 0);

        float colors[] = {
                1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 1.0f,
                0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 0.0f,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
    }

    void draw() override {
        auto view_matrix = viewport.local_fixed_ratio_to_standard_square();
        auto vpm = glm::mat4(view_matrix);

        this->shader_program->use();
        shader_program->setUniform("vpm", vpm);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, 8);

        this->shader_program->unuse();
    }

private:

    void preload_shader_program() {
        if (!cube_shader_program) {
            cube_shader_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("cube.vert"), GL_VERTEX_SHADER},
                    Shader{SHADER_PATH("cube.geom"), GL_GEOMETRY_SHADER},
                    Shader{SHADER_PATH("cube.frag"), GL_FRAGMENT_SHADER}
            });
        }
        this->shader_program = cube_shader_program;
    }

    GLuint vao;
    GLuint vbo_point;
    GLuint vbo_color;

    static std::shared_ptr<ShaderProgram> cube_shader_program;
};

std::shared_ptr<ShaderProgram> CubeDrawable::cube_shader_program = nullptr;

void register_cube(Application &application) {
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

    application.add_component(
            std::make_shared<CubeDrawable>(viewport_function(application)),
            viewport_function
    );
}
