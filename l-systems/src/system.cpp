#include "system.hpp"
#include "properties.hpp"
#include "grammar.h"
#include "GLFW/glfw3.h"
#include "baseline.hpp"
#include "args.hpp"
#include "debug.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>


constexpr float ROTATION_SPEED = 0.5f;
constexpr float ROTATION_DISTANCE = 50.0f;
constexpr float ROTATION_HEIGHT = 1.0f;
constexpr float DOWNSET_FACTOR = 10.0f;


class SystemDrawable : public Drawable {
public:

    explicit SystemDrawable(const Viewport &viewport, const std::shared_ptr<View> &view, const std::string &grammarPath)
            : Drawable(viewport, view), grammar(load_grammar(grammarPath)) {
        grammar->print();
        downset = grammar->get_property_float("downset");
    }

    ~SystemDrawable() override = default;

    void init() override {
        preload_shader_program();
        preload_compute_shader_program();

        std::string result = grammar->cpu_produce();
        // std::cout << "Result: " << result << std::endl;

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
//        std::vector<glm::mat4> instances = TempSpace::grammar_instances(grammar);
//
//        instance_translations_count = instances.size();


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

    void move_down() {
        downset += DOWNSET_FACTOR;
    }

    void move_up() {
        downset -= DOWNSET_FACTOR;
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

        if (!instance_detector_program) {
            instance_detector_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/instance_detector.comp"), GL_COMPUTE_SHADER}
            });
        }
        this_instance_detector_program = instance_detector_program;

        if (!matrix_filler_program) {
            matrix_filler_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/matrix_filler.comp"), GL_COMPUTE_SHADER}
            });
        }
        this_matrix_filler_program = matrix_filler_program;

        if (!matrix_multiplier_program) {
            matrix_multiplier_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/prefix_matrix.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_matrix_multiplier_program = matrix_multiplier_program;

        if (!instance_placer_program) {
            instance_placer_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/instance_placer.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_instance_placer_program = instance_placer_program;

        if (!leaf_detector_program) {
            leaf_detector_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/leaf_detector.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_leaf_detector_program = leaf_detector_program;

        if (!leaf_edge_detector_program) {
            leaf_edge_detector_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("leafs/leaf_edge_detector.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_leaf_edge_detector_program = leaf_edge_detector_program;

        if (!leaf_begin_detector_program) {
            leaf_begin_detector_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("leafs/leaf_begin_detector.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_leaf_begin_detector_program = leaf_begin_detector_program;
    }

    void prepare_productions_ssbo() {
        using SIZE = int32_t;
        SIZE required_ascii_size = 128;
        std::vector<SIZE> productions_offsets(required_ascii_size, -1);
        std::vector<SIZE> productions_sizes(required_ascii_size, -1);

        SIZE length_so_far = 0;
        for (const auto &[from, to_and_lookback]: *grammar->get_productions()) {
            auto &[to, _] = to_and_lookback;
            productions_offsets[from] = length_so_far;
            productions_sizes[from] = to.size();
            length_so_far += to.size();
        }

        std::vector<SIZE> productions(length_so_far);
        std::vector<SIZE> look_backs(length_so_far);
        for (const auto &[from, to_and_lookback]: *grammar->get_productions()) {
            auto &[to, lookback] = to_and_lookback;
            std::copy(to.begin(), to.end(), productions.begin() + productions_offsets[from]);
            std::copy(lookback.begin(), lookback.end(), look_backs.begin() + productions_offsets[from]);
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

        glGenBuffers(1, &ssbo_productions_look_backs);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_productions_look_backs);
        glBufferData(GL_SHADER_STORAGE_BUFFER, look_backs.size() * sizeof(decltype(look_backs)::value_type),
                     look_backs.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_productions_look_backs);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &ssbo_previous_result_buffer);
        glGenBuffers(1, &ssbo_offset_buffer);
        glGenBuffers(1, &ssbo_next_result_buffer);
        glGenBuffers(1, &ssbo_previous_look_back_buffer);
        glGenBuffers(1, &ssbo_next_look_back_buffer);
        glGenBuffers(1, &transformations_input_ssbo);
        glGenBuffers(1, &transformations_output_ssbo);
        glGenBuffers(1, &ssbo_input_jumps);
        glGenBuffers(1, &ssbo_output_jumps);
        glGenBuffers(1, &ssbo_translations);
        glGenBuffers(1, &ssbo_is_a_leaf_output);
        glGenBuffers(1, &ssbo_leaf_edge_counter);
        glGenBuffers(1, &ssbo_leaf_begin_counter);
    }

    void run_compute() {
        run_str_compute();
        run_instance_compute();
    }

    void run_str_compute() {

        // BEGIN init data - put axiom into previous result buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
        auto axiom_data = grammar->get_axiom()->get_as_opengl_data();
        auto axiom_size = axiom_data.size();
        glBufferData(GL_SHADER_STORAGE_BUFFER, axiom_data.size() * sizeof(decltype(axiom_data)::value_type),
                     axiom_data.data(), GL_STATIC_DRAW);

        // put axiom look_back into previous look_back buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_look_back_buffer);
        auto axiom_look_back_data = grammar->get_axiom()->look_back;
        glBufferData(GL_SHADER_STORAGE_BUFFER, axiom_look_back_data.size() * sizeof(decltype(axiom_look_back_data)::value_type),
                     axiom_look_back_data.data(), GL_STATIC_DRAW);

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

            // print prev lookback
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_look_back_buffer);
//            auto prev_look_back_data = (int32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "prev_lb[" << i << "] = " << prev_look_back_data[i] << std::endl;
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
//            auto ps_data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "prefix_sum[" << i << "] = " << ps_data[i] << std::endl;
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

            // make space for next result buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_look_back_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_productions_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_productions_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_productions);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_previous_result_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_offset_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_next_result_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_productions_look_backs);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_previous_look_back_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_next_look_back_buffer);

            glDispatchCompute(result_buffer_size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            this_production_shader_program->unuse();

            // print new result
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
//            auto next_data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "next[" << i << "] = " << next_data[i] << "(" << (char) next_data[i] << ")" << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

            // print new lookback
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_look_back_buffer);
//            auto look_back_data = (int32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "look_back[" << i << "] = " << look_back_data[i] << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


            // copy data from next result buffer to previous result buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glCopyNamedBufferSubData(ssbo_next_result_buffer, ssbo_previous_result_buffer, 0, 0,
                                     result_buffer_size * sizeof(uint32_t));

            // copy look_backs from next look_back buffer to previous look_back buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_look_back_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glCopyNamedBufferSubData(ssbo_next_look_back_buffer, ssbo_previous_look_back_buffer, 0, 0,
                                     result_buffer_size * sizeof(uint32_t));

//            // print ssbo_previous_result_buffer
//            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
//            data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//            for (size_t i = 0; i < result_buffer_size; i++) {
//                std::cout << "prev[" << i << "] = " << data[i] << "(" << (char) data[i] << ")" << std::endl;
//            }
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        ssbo_word_length = result_buffer_size;

        // check data
//
//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_result_buffer);
//        auto *data = (uint32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//        std::cout << "\"";
//        for (size_t i = 0; i < result_buffer_size; i++) {
//            std::cout << (char) data[i];
////            std::cout << "data[" << i << "] = " << data[i] << "(" << (char) data[i] << ")" << std::endl;
//        }
//        std::cout << "\"" << std::endl;
//        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
//
//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_previous_look_back_buffer);
//        auto *look_back_data = (int32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//        std::cout << "[";
//        for (size_t i = 0; i < result_buffer_size; i++) {
//            std::cout << look_back_data[i] << ", ";
////            std::cout << "lb[" << i << "] = " << look_back_data[i] << std::endl;
//        }
//        std::cout << "]" << std::endl;
//        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    void run_instance_compute() {
        // ssbo_previous_result_buffer, ssbo_previous_look_back_buffer
        // produced word, and produced look_back

        init_transformations(ssbo_previous_result_buffer, ssbo_word_length);
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

        glDeleteBuffers(1, &ssbo_output);

        this_prefix_sum_shader_program->unuse();
    }

    void init_transformations(GLuint ssbo, size_t size) {

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_is_a_leaf_output);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_leaf_edge_counter);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_leaf_begin_counter);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

        this_leaf_detector_program->use();
        this_leaf_detector_program->setUniform("char_to_find_one", (int) '{');
        this_leaf_detector_program->setUniform("char_to_find_minus_one", (int) '}');
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_is_a_leaf_output);

        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_leaf_detector_program->unuse();

        run_prefix_sum(ssbo_is_a_leaf_output, size);

        this_leaf_edge_detector_program->use();
        this_leaf_edge_detector_program->setUniform("char_to_find", (int) 'f');
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_is_a_leaf_output);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_leaf_edge_counter);
        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_leaf_edge_detector_program->unuse();

        run_prefix_sum(ssbo_leaf_edge_counter, size);

        this_leaf_begin_detector_program->use();
        this_leaf_begin_detector_program->setUniform("char_to_find", (int) '{');
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_leaf_begin_counter);
        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_leaf_begin_detector_program->unuse();

        run_prefix_sum(ssbo_leaf_begin_counter, size);

        // reuse ssbo_next_result_buffer for counting
        // resize buffer to the size of the word
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

        this_instance_detector_program->use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_next_result_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_is_a_leaf_output);

        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_instance_detector_program->unuse();

        run_prefix_sum(ssbo_next_result_buffer, size);

        // get the count of instances
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
        int32_t drawable_instances_count;
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (size - 1) * sizeof(uint32_t), sizeof(uint32_t), &drawable_instances_count);

//        std::cout << "Drawable instances count: " << drawable_instances_count << std::endl;

        this_matrix_filler_program->use();
        auto step = grammar->get_property_float("step");
        auto delta_in_angles = grammar->get_property_float("delta");
        auto delta = glm::radians(delta_in_angles);
        this_matrix_filler_program->setUniform("step", step);
        this_matrix_filler_program->setUniform("delta", delta);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, transformations_input_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, transformations_input_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        this_matrix_filler_program->unuse();

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, transformations_output_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);

        // init input jumps to be LookBack table
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input_jumps);
        // copy ssbo_previous_look_back_buffer into ssbo_input_jumps
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);
        glCopyNamedBufferSubData(ssbo_previous_look_back_buffer, ssbo_input_jumps, 0, 0, size * sizeof(int32_t));

        // init output jumps to be LookBack table
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_output_jumps);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);


        this_matrix_multiplier_program->use();
        // bind buffers for prefix matrix run
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, transformations_input_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, transformations_output_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_input_jumps);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_output_jumps);

        // overestimate epochs to be ceil of log2 of size
        // no more epochs needed than all nodes in the tree
        auto epochs = (size_t) ceil(log2((double) size));
        for (size_t epoch = 0; epoch <= epochs; ++epoch) {

            // run the compute
            this_matrix_multiplier_program->setUniform("epoch", (int) epoch);
            glDispatchCompute(size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // copy output into output for both jumps and transformations
            glCopyNamedBufferSubData(ssbo_output_jumps, ssbo_input_jumps, 0, 0, size * sizeof(int32_t));
            glCopyNamedBufferSubData(transformations_output_ssbo, transformations_input_ssbo, 0, 0, size * sizeof(glm::mat4));
        }
        this_matrix_multiplier_program->unuse();

        // check matrices in transformations_input_ssbo
//         glBindBuffer(GL_SHADER_STORAGE_BUFFER, transformations_input_ssbo);
//         auto matrices = (glm::mat4 *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//         for (size_t i = 0; i < size; i++) {
//            std::cout << "Matrix[" << i << "]: " << std::endl;
//            for (size_t j = 0; j < 4; j++) {
//                for (size_t k = 0; k < 4; k++) {
//                    std::cout << matrices[i][j][k] << " ";
//                }
//                std::cout << std::endl;
//            }
//        }
//        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        instance_translations_count = drawable_instances_count;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_translations);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instance_translations_count * sizeof(glm::mat4),
                     nullptr, GL_STATIC_DRAW);

        // place instances
        this_instance_placer_program->use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, transformations_input_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_translations);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_next_result_buffer);

        glm::mat4 common_turtle_matrix = glm::transpose(glm::mat4{
            0, 1, 0, 0,
            -1, 0, 0, 0,
            0, 0, 1, 0,
            0, -downset, 0, 1
        });

        auto move_down = glm::translate(glm::mat4(1.0), glm::vec3(-0.5f, 0.0, 0.0f));
        auto scale_y_by_step = glm::scale(glm::mat4(1.0), glm::vec3(-step, 1.0f, 1.0f));
        auto move_back_up = glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.0f, 0.0f));
        auto translation = move_down * scale_y_by_step * move_back_up;

        this_instance_placer_program->setUniform("common_turtle_matrix", common_turtle_matrix);
        this_instance_placer_program->setUniform("common_cube_matrix", translation);

        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_instance_placer_program->unuse();

//        std::cout << "Size of the string produced: " << size << std::endl;
//        std::cout << "Number of instances: " << drawable_instances_count << std::endl;

        // std::exit(1);

        // TEST

//        std::vector<glm::mat4> expected_instances = TempSpace::grammar_instances(grammar);
//        std::cout << "Expected instances count: " << expected_instances.size() << std::endl;

//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_translations);
//        auto *data = (glm::mat4 *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
//        for (size_t i = 0; i < instance_translations_count; i++) {
//            std::cout << "Instance[" << i << "]: " << std::endl;
//            print_mat4(data[i]);
//            std::cout << "Expected instance[" << i << "]: " << std::endl;
//            print_mat4(expected_instances[i]);
//        }

//        std::exit(1);

        // END test
    }

    GrammarPtr grammar;

    GLuint vao{};
    GLuint vbo_point{};
    GLuint vbo_color{};
    GLuint ibo{};
    GLuint ssbo_translations{};
    GLuint instance_translations_count = -1;

    float downset{};

    size_t ssbo_word_length{};

    GLuint ssbo_productions_offsets{};
    GLuint ssbo_productions_sizes{};
    GLuint ssbo_productions_look_backs{};
    GLuint ssbo_productions{};
    GLuint ssbo_previous_result_buffer{};
    GLuint ssbo_offset_buffer{};
    GLuint ssbo_next_result_buffer{};
    GLuint ssbo_previous_look_back_buffer{};
    GLuint ssbo_next_look_back_buffer{};
    GLuint transformations_input_ssbo{};
    GLuint transformations_output_ssbo{};
    GLuint ssbo_input_jumps{};
    GLuint ssbo_output_jumps{};
    GLuint ssbo_is_a_leaf_output{};
    GLuint ssbo_leaf_edge_counter{};
    GLuint ssbo_leaf_begin_counter{};

    std::shared_ptr<ShaderProgram> this_production_shader_program;
    std::shared_ptr<ShaderProgram> this_prefix_sum_shader_program;
    std::shared_ptr<ShaderProgram> this_size_shader_program;
    std::shared_ptr<ShaderProgram> this_instance_detector_program;
    std::shared_ptr<ShaderProgram> this_matrix_filler_program;
    std::shared_ptr<ShaderProgram> this_matrix_multiplier_program;
    std::shared_ptr<ShaderProgram> this_instance_placer_program;
    std::shared_ptr<ShaderProgram> this_leaf_detector_program;
    std::shared_ptr<ShaderProgram> this_leaf_edge_detector_program;
    std::shared_ptr<ShaderProgram> this_leaf_begin_detector_program;

    static std::shared_ptr<ShaderProgram> system_shader_program;
    static std::shared_ptr<ShaderProgram> production_shader_program;
    static std::shared_ptr<ShaderProgram> prefix_sum_shader_program;
    static std::shared_ptr<ShaderProgram> size_shader_program;
    static std::shared_ptr<ShaderProgram> instance_detector_program;
    static std::shared_ptr<ShaderProgram> matrix_filler_program;
    static std::shared_ptr<ShaderProgram> matrix_multiplier_program;
    static std::shared_ptr<ShaderProgram> instance_placer_program;
    static std::shared_ptr<ShaderProgram> leaf_detector_program;
    static std::shared_ptr<ShaderProgram> leaf_edge_detector_program;
    static std::shared_ptr<ShaderProgram> leaf_begin_detector_program;
};

std::shared_ptr<ShaderProgram> SystemDrawable::system_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::production_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::prefix_sum_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::size_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::instance_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::matrix_filler_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::matrix_multiplier_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::instance_placer_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_edge_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_begin_detector_program = nullptr;

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

    application.get_window().set_key_callback([&application, rotation_view, the_system](int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(application.get_window().get_window(), GLFW_TRUE);
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            rotation_view->switch_on_off();
        }

        static bool is_up_pressed = false;
        static bool is_down_pressed = false;

        if (key == GLFW_KEY_UP) {
            if (action == GLFW_PRESS) {
                is_up_pressed = true;
            } else if (action == GLFW_RELEASE) {
                is_up_pressed = false;
            }
        }

        if (is_up_pressed) {
            the_system->move_up();
        }

        if (key == GLFW_KEY_DOWN) {
            if (action == GLFW_PRESS) {
                is_down_pressed = true;
            } else if (action == GLFW_RELEASE) {
                is_down_pressed = false;
            }
        }

        if (is_down_pressed) {
            the_system->move_down();
        }
    });

    application.get_window().set_scroll_callback([rotation_view](double, double y) {
        rotation_view->zoom(static_cast<float>(-y));
    });
}
