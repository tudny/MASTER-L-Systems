#include "system.hpp"
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


class SystemDrawable : public Drawable {
public:

    explicit SystemDrawable(const Viewport &viewport, const std::shared_ptr<View> &view, const std::string &grammarPath)
            : Drawable(viewport, view), grammar(load_grammar(grammarPath)) {
        grammar->print();
    }

    ~SystemDrawable() override = default;

    void init() override {
        preload_shader_program();
        preload_compute_shader_program();

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

        prepare_productions_ssbo();
    }

    void draw() override {

        run_compute();

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

        // rebind ssbo to the same binding point after compute shader
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_translations);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_translations);

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
        if (!system_shader_program) {
            system_shader_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("system.vert"), GL_VERTEX_SHADER},
                    Shader{SHADER_PATH("system.geom"), GL_GEOMETRY_SHADER},
                    Shader{SHADER_PATH("system.frag"), GL_FRAGMENT_SHADER}
            });
        }
        this->shader_program = system_shader_program;
    }

    void preload_compute_shader_program() {
        if (!production_shader_program) {
            production_shader_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/production.comp"), GL_COMPUTE_SHADER}
            });
        }
        this_production_shader_program = production_shader_program;

        if (!prefix_sum_shader_program) {
            prefix_sum_shader_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/prefix_sum.comp"), GL_COMPUTE_SHADER}
            });
        }
        this_prefix_sum_shader_program = prefix_sum_shader_program;

        if (!size_shader_program) {
            size_shader_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/size.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_size_shader_program = size_shader_program;
    }

    void prepare_productions_ssbo() {
        using SIZE = uint32_t;
        SIZE required_ascii_size = 128;
        std::vector<SIZE> productions_offsets(required_ascii_size, -1);
        std::vector<SIZE> productions_sizes(required_ascii_size, -1);

        SIZE length_so_far = 0;
        for (const auto &[from, to]: *grammar->get_productions()) {
            productions_offsets[from] = length_so_far;
            productions_sizes[from] = to.size();
            length_so_far += to.size();
        }

        std::vector<SIZE> productions(length_so_far);
        for (const auto &[from, to]: *grammar->get_productions()) {
            std::copy(to.begin(), to.end(), productions.begin() + productions_offsets[from]);
        }

        glGenBuffers(1, &ssbo_productions_offsets);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_productions_offsets);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     productions_offsets.size() * sizeof(decltype(productions_offsets)::value_type),
                     productions_offsets.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_productions_offsets);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &ssbo_productions_sizes);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_productions_sizes);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     productions_sizes.size() * sizeof(decltype(productions_sizes)::value_type),
                     productions_sizes.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_productions_sizes);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &ssbo_productions);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_productions);
        glBufferData(GL_SHADER_STORAGE_BUFFER, productions.size() * sizeof(decltype(productions)::value_type),
                     productions.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_productions);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void run_compute() {

        GLuint ssbo_previous_result_buffer;
        glGenBuffers(1, &ssbo_previous_result_buffer);
        GLuint ssbo_offset_buffer;
        glGenBuffers(1, &ssbo_offset_buffer);
        GLuint ssbo_next_result_buffer;
        glGenBuffers(1, &ssbo_next_result_buffer);

        // BEGIN init data - put axiom into previous result buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
        auto axiom_data = grammar->get_axiom()->get_as_opengl_data();
        auto axiom_size = axiom_data.size();
        glBufferData(GL_SHADER_STORAGE_BUFFER, axiom_data.size() * sizeof(decltype(axiom_data)::value_type),
                     axiom_data.data(), GL_STATIC_DRAW);

        size_t result_buffer_size = axiom_size;
        // END init data

        size_t epoch_num = grammar->get_property_size_t("depth");

        for (size_t epoch = 0; epoch < epoch_num; ++epoch) {
//            std::cout << "===========================================================" << std::endl;
//            std::cout << "Epoch: " << epoch << std::endl;

            // print contents of ssbo_previous_result_buffer
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
//            auto *data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "prev[" << i << "] = " << data[i] << "(" << (char) data[i] << ")" << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            // make a copy of ssbo_previous_result_buffer into ssbo_offset_buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_offset_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glCopyNamedBufferSubData(ssbo_previous_result_buffer, ssbo_offset_buffer, 0, 0,
                                     result_buffer_size * sizeof(uint32_t));
//            std::cout << "New size of offset buffer: " << result_buffer_size << std::endl;

            // map letters into production sizes in the copy
            this_size_shader_program->use();
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_productions_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_productions_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_productions);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_offset_buffer);
            glDispatchCompute(result_buffer_size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            this_size_shader_program->unuse();

            // print mapped sizes
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_offset_buffer);
//            data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "size[" << i << "] = " << data[i] << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            // run prefix sum on the copy
            run_prefix_sum(ssbo_offset_buffer, result_buffer_size);

            // print prefix sum
//            data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "prefix_sum[" << i << "] = " << data[i] << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            // get buffer element at the end that will be the size of the result buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_offset_buffer);
            // here we change the stored value to the size of the result buffer
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (result_buffer_size - 1) * sizeof(uint32_t), sizeof(uint32_t),
                               &result_buffer_size);

//            std::cout << "Result buffer size: " << result_buffer_size << std::endl;

            // generate productions for each letter into next result buffer
            this_production_shader_program->use();

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_productions_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_productions_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_productions);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_previous_result_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_offset_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_next_result_buffer);

            glDispatchCompute(result_buffer_size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            this_production_shader_program->unuse();

            // print new result
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
//            data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "next[" << i << "] = " << data[i] << "(" << (char) data[i] << ")" << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            // copy data from next result buffer to previous result buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glCopyNamedBufferSubData(ssbo_next_result_buffer, ssbo_previous_result_buffer, 0, 0,
                                     result_buffer_size * sizeof(uint32_t));

//            // print ssbo_previous_result_buffer
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
//            data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "prev[" << i << "] = " << data[i] << "(" << (char) data[i] << ")" << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        // check data

//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
//        auto *data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//        for (size_t i = 0; i < result_buffer_size; i++) {
//            std::cout << "data[" << i << "] = " << data[i] << "(" << (char) data[i] << ")" << std::endl;
//        }
    }

    void run_prefix_sum(GLuint ssbo, size_t size) {
        this_prefix_sum_shader_program->use();

        GLuint ssbo_output;
        glGenBuffers(1, &ssbo_output);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_output);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

        auto steps = (size_t) ceil(log2((double) size));
//        std::cout << "Running prefix sum with " << steps << " steps" << std::endl;

        for (size_t step = 1; step <= steps; ++step) {

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_output);

//            std::cout << "Step: " << step << std::endl;
            this_prefix_sum_shader_program->setUniform("step", (int) step);
            glDispatchCompute(size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // put output back into input
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            glCopyNamedBufferSubData(ssbo_output, ssbo, 0, 0, size * sizeof(uint32_t));
        }

        this_prefix_sum_shader_program->unuse();
    }

    GrammarPtr grammar;

    GLuint vao{};
    GLuint vbo_point{};
    GLuint vbo_color{};
    GLuint ibo{};
    GLuint ssbo_translations{};
    GLuint instance_translations_count = -1;

    GLuint ssbo_productions_offsets{};
    GLuint ssbo_productions_sizes{};
    GLuint ssbo_productions{};

    std::shared_ptr<ShaderProgram> this_production_shader_program;
    std::shared_ptr<ShaderProgram> this_prefix_sum_shader_program;
    std::shared_ptr<ShaderProgram> this_size_shader_program;

    static std::shared_ptr<ShaderProgram> system_shader_program;
    static std::shared_ptr<ShaderProgram> production_shader_program;
    static std::shared_ptr<ShaderProgram> prefix_sum_shader_program;
    static std::shared_ptr<ShaderProgram> size_shader_program;
};

std::shared_ptr<ShaderProgram> SystemDrawable::system_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::production_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::prefix_sum_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::size_shader_program = nullptr;

void register_system(Application &application, ContextPtr &context) {
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

    auto the_system = std::make_shared<SystemDrawable>(
            viewport_function(application),
            rotation_view,
            context->grammar_path
    );

    application.add_component(
            the_system,
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
