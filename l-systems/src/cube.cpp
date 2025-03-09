#include "cube.hpp"
#include "properties.hpp"
#include "grammar.h"
#include "GLFW/glfw3.h"
#include "baseline.hpp"
#include "args.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>


constexpr float ROTATION_SPEED = 0.5f;
constexpr float ROTATION_DISTANCE = 50.0f;
constexpr float ROTATION_HEIGHT = 1.0f;


class CubeDrawable : public Drawable {
public:

    explicit CubeDrawable(const Viewport &viewport, const std::shared_ptr<View> &view, const std::string &grammarPath)
            : Drawable(viewport, view), grammar(load_grammar(grammarPath)) {
        grammar->print();
    }

    ~CubeDrawable() override = default;

    void init() override {
        preload_shader_program();

        std::string result = grammar->cpu_produce();
        std::cout << "Result: " << result << std::endl;

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

//        // set color to brown
        for (int i = 0; i < 8; i++) {
            colors[i * 3] = 0.5f;
            colors[i * 3 + 1] = 0.35f;
            colors[i * 3 + 2] = 0.05f;
        }

        glGenBuffers(1, &vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
        shader_program->setAttribute("color", 3, 0, 0);

        GLuint indices[] = {
                0, 1, 2,
                0, 2, 3,
                3, 2, 6,
                3, 6, 7,
                5, 7, 6,
                4, 7, 5,
                1, 4, 5,
                0, 4, 1,
                3, 7, 4,
                0, 3, 4,
                1, 5, 6,
                1, 6, 2,
        };

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

//        std::vector<glm::mat4> instances = {
//                glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.0f)), glm::vec3(5.0f)),
//                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, -10.0f)),
//            glm::mat4(1.0f),
//        };

//        std::vector<glm::mat4> instances = TempSpace::sample_instances();
        std::vector<glm::mat4> instances = TempSpace::grammar_instances(grammar);

        instance_translations_count = instances.size();

        glGenBuffers(1, &ssbo_translations);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_translations);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instances.size() * sizeof(decltype(instances)::value_type),
                     instances.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_translations);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glEnableVertexAttribArray(0);
    }

    void draw() override {

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        double time = glfwGetTime();
        glm::vec4 eye_pos = this->get_view()->get_eye_pos();
        glm::mat4 projection = viewport.make_3d_projection();
        glm::mat4 view = this->get_view()->get_view_matrix();
        // move up and down as sin(time)
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, sin(time * 10), 0.0f));
        model = glm::mat4(1.0f);
        auto pvm = projection * view * model;

        this->shader_program->use();

        shader_program->setUniform("pvm", pvm);
        shader_program->setUniform("eyepos", eye_pos);
        shader_program->setUniform("ls_ambient", glm::vec3(0.1, 0.1, 0.1));
        shader_program->setUniform("ls_position", glm::vec4(2, 2, 2, 1.0));
        shader_program->setUniform("ls_attenuation", glm::vec3(0.2f, 0.2f, 0.2f));
        shader_program->setUniform("ls_direct", glm::vec3(0.0, 3.0, 0.0));

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

    GrammarPtr grammar;

    GLuint vao{};
    GLuint vbo_point{};
    GLuint vbo_color{};
    GLuint ibo{};
    GLuint ssbo_translations{};
    GLuint instance_translations_count = -1;

    static std::shared_ptr<ShaderProgram> cube_shader_program;
};

std::shared_ptr<ShaderProgram> CubeDrawable::cube_shader_program = nullptr;

void register_cube(Application &application, ContextPtr &context) {
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
            ROTATION_SPEED,
            ROTATION_DISTANCE,
            ROTATION_HEIGHT
    );

    auto the_cube = std::make_shared<CubeDrawable>(
            viewport_function(application),
            rotation_view,
            context->grammar_path
    );

    application.add_component(
            the_cube,
            viewport_function
    );

    application.get_window().set_key_callback([&application, rotation_view](int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(application.get_window().get_window(), GLFW_TRUE);
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            rotation_view->switch_on_off();
        }
    });

    application.get_window().set_scroll_callback([rotation_view](double, double y) {
        rotation_view->zoom(static_cast<float>(-y));
    });
}
