#include <iostream>
#include "cube.hpp"
#include "properties.hpp"
#include "errors.hpp"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


constexpr float ROTATION_SPEED = 0.5f;
constexpr float ROTATION_DISTANCE = 15.0f;
constexpr float ROTATION_HEIGHT = 1.0f;
constexpr glm::vec3 ROTATION_CENTER = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr glm::vec3 ROTATION_UP = glm::vec3(0.0f, 0.0f, 1.0f);


class CubeDrawable : public Drawable {
public:

    explicit CubeDrawable(const Viewport &viewport) : Drawable(viewport) {}

    ~CubeDrawable() override = default;

    void init() override {
        preload_shader_program();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

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

        glGenBuffers(1, &vbo_point);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_point);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        shader_program->setAttribute("position", 3, 0, 0);

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

        glGenBuffers(1, &vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
        shader_program->setAttribute("color", 3, 0, 0);

        GLuint indices[] = {
                0, 1, 2,
                2, 0, 3,
                3, 2, 6,
                6, 3, 7,
                7, 6, 5,
                5, 7, 4,
                4, 5, 1,
                1, 0, 4,
                4, 7, 3,
                3, 0, 4,
                1, 5, 6,
                6, 2, 1,
        };

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        float instances_translations[] = {
                0.0f, 0.0f, 0.0f,
                3.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 4.0f,
        };
        size_t instances_translations_size = sizeof(instances_translations) / sizeof(float);
        assert(instances_translations_size % 3 == 0);
        instances_translations_size /= 3;
        assert(instances_translations_size == instance_translations_count);

        glGenBuffers(1, &vbo_instance_translations);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance_translations);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * instances_translations_size, instances_translations, GL_STATIC_DRAW);
        shader_program->setAttribute("translation", 3, 0, 0);
        glVertexAttribDivisor(2, 1);

        glEnableVertexAttribArray(0);
    }

    void draw() override {

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        double time = glfwGetTime() * ROTATION_SPEED;
        glm::vec4 eye_pos = glm::vec4(
                ROTATION_DISTANCE * sin(time),
                ROTATION_DISTANCE * cos(time),
                ROTATION_HEIGHT, 1.0
        );
        glm::mat4 projection = viewport.make_3d_projection();
        glm::mat4 view = glm::lookAt(
                glm::vec3(eye_pos),
                ROTATION_CENTER,
                ROTATION_UP
        );
        // move up and down as sin(time)
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, sin(time * 10)));

        auto pvm = projection * view * model;

        this->shader_program->use();

        shader_program->setUniform("pvm", pvm);
        shader_program->setUniform("eyepos", eye_pos);
        shader_program->setUniform("ls_ambient", glm::vec3(1.0, 0.1, 0.1));
        shader_program->setUniform("ls_position", glm::vec4(0, 2, 0, 1.0));
        shader_program->setUniform("ls_attenuation", glm::vec3(0.2f, 0.2f, 0.2f));
        shader_program->setUniform("ls_direct", glm::vec3(0.0, 1.0, 0.0));

        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, instance_translations_count);

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

    GLuint vao{};
    GLuint vbo_point{};
    GLuint vbo_color{};
    GLuint ibo{};
    GLuint vbo_instance_translations{};
    constexpr static size_t instance_translations_count = 3;

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
