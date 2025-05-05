#include "system.hpp"
#include "properties.hpp"
#include "grammar.h"
#include "GLFW/glfw3.h"
#include "args.hpp"
#include "debug.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>


constexpr float ROTATION_SPEED = 0.5f;
constexpr float ROTATION_DISTANCE = 50.0f;
constexpr float ROTATION_HEIGHT = 1.0f;
constexpr float DOWNSET_FACTOR = 10.0f;
constexpr bool MARK_LEAF = false;


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

        glGenVertexArrays(1, &vao_leaf);

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

        glEnableVertexAttribArray(0);

        prepare_new_productions_ssbo();

        glGenBuffers(1, &ssbo_previous_result_buffer);
        glGenBuffers(1, &ssbo_offset_buffer);
        glGenBuffers(1, &ssbo_production_index_buffer);
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
        glGenBuffers(1, &ssbo_leaf_positions_vec4);
        glGenBuffers(1, &ssbo_leaf_color_int);
        glGenBuffers(1, &ssbo_leaf_index_array);
        glGenBuffers(1, &ssbo_colors__index);

        glGenBuffers(1, &ssbo_color_buffer_input);
        glGenBuffers(1, &ssbo_color_buffer_output);
        glGenBuffers(1, &ssbo_length_buffer_input);
        glGenBuffers(1, &ssbo_length_buffer_output);
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
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_colors__values);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_colors__index);

        set_light_and_pv(this->shader_program, pvm, eye_pos);

        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, instance_translations_count);

        this->shader_program->unuse();

        run_leaf_draw(pvm, eye_pos);
    }

    void
    set_light_and_pv(const std::shared_ptr<ShaderProgram> &program, const glm::mat4 &pvm, const glm::vec4 &eye_pos) {
        program->setUniform("pvm", pvm);
        program->setUniform("eyepos", eye_pos);
        program->setUniform("ls_ambient", glm::vec3(0.6, 0.6, 0.6));
        program->setUniform("ls_position", glm::vec4(2, 2, 2, .1));
        program->setUniform("ls_attenuation", glm::vec3(0.2f, 0.2f, 0.2f));
        program->setUniform("ls_direct", glm::vec3(1.0, 1.0, 1.0));
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

        if (!leaf_position_placer_program) {
            leaf_position_placer_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("leafs/leaf_position_placer.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_leaf_position_placer_program = leaf_position_placer_program;

        if (!leaf_program) {
            leaf_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("leafs/leaf.vert"), GL_VERTEX_SHADER},
                    Shader{SHADER_PATH("system.geom"), GL_GEOMETRY_SHADER},
                    Shader{SHADER_PATH("system.frag"), GL_FRAGMENT_SHADER},
            });
        }

        this_leaf_program = leaf_program;

        if (!find_production_and_size_program) {
            find_production_and_size_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/find_production_and_size.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_find_production_and_size_program = find_production_and_size_program;

        if (!prefix_job_on_tree_program) {
            prefix_job_on_tree_program = std::make_shared<ShaderProgram>(std::initializer_list<Shader>{
                    Shader{SHADER_PATH("productions/prefix_job_on_tree.comp"), GL_COMPUTE_SHADER}
            });
        }

        this_prefix_job_on_tree_program = prefix_job_on_tree_program;
    }

    void prepare_new_productions_ssbo() {
        auto productions_gl_data = grammar->get_opengl_ready_productions();
        std::vector<std::pair<GLuint *, OpenGLReadyProductionDataType>> mappings = {
                {&ssbo_new_productions__predecessors,          productions_gl_data.predecessors},
                {&ssbo_new_productions__look_back,             productions_gl_data.look_back},
                {&ssbo_new_productions__successors_sizes,      productions_gl_data.successors_sizes},
                {&ssbo_new_productions__successors_offsets,    productions_gl_data.successors_offsets},
                {&ssbo_new_productions__successors_data,       productions_gl_data.successors_data},
                {&ssbo_new_productions__left_context_sizes,    productions_gl_data.left_context_sizes},
                {&ssbo_new_productions__left_context_offsets,  productions_gl_data.left_context_offsets},
                {&ssbo_new_productions__left_context_data,     productions_gl_data.left_context_data},
                {&ssbo_new_productions__right_context_sizes,   productions_gl_data.right_context_sizes},
                {&ssbo_new_productions__right_context_offsets, productions_gl_data.right_context_offsets},
                {&ssbo_new_productions__right_context_data,    productions_gl_data.right_context_data},
                {&ssbo_new_productions__ignored,               productions_gl_data.ignored}
        };

        this->productions_count = grammar->get_raw_productions().size();
        this->ignored_count = grammar->get_ignored()->ignored.size();

        for (const auto &[buffer, data]: mappings) {
            glGenBuffers(1, buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, *buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, data->size() * sizeof(int32_t),
                         data->data(), GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        glGenBuffers(1, &ssbo_colors__values);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_colors__values);
        glBufferData(GL_SHADER_STORAGE_BUFFER, grammar->get_colors().size() * sizeof(ColorT),
                     grammar->get_colors().data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_colors__values);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     axiom_look_back_data.size() * sizeof(decltype(axiom_look_back_data)::value_type),
                     axiom_look_back_data.data(), GL_STATIC_DRAW);

        size_t result_buffer_size = axiom_size;
        // END init data

        size_t epoch_num_max = grammar->get_property_size_t("depth");

        size_t epoch_num = epoch_num_max;
        if (grammar->get_property_size_t("dynamicmax") == 1) {
            epoch_num = std::min(static_cast<size_t>(glfwGetTime()), epoch_num_max);
        } else if (grammar->get_property_size_t("dynamic") == 1) {
            epoch_num = (static_cast<size_t>(glfwGetTime()) % epoch_num_max) + 1;
        }

        for (size_t epoch = 0; epoch < epoch_num; ++epoch) {
            // make a copy of ssbo_previous_result_buffer into ssbo_offset_buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_offset_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glCopyNamedBufferSubData(ssbo_previous_result_buffer, ssbo_offset_buffer, 0, 0,
                                     result_buffer_size * sizeof(uint32_t));

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_production_index_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

            this_find_production_and_size_program->use();
            this_find_production_and_size_program->setUniform("wholeInputSize", (int) result_buffer_size);
            this_find_production_and_size_program->setUniform("numberOfProductions", (int) productions_count);
            this_find_production_and_size_program->setUniform("ignoredSize", (int) ignored_count);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_new_productions__predecessors);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_new_productions__look_back);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_new_productions__successors_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_new_productions__successors_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_new_productions__successors_data);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_new_productions__left_context_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_new_productions__left_context_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_new_productions__left_context_data);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_new_productions__right_context_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_new_productions__right_context_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_new_productions__right_context_data);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssbo_previous_result_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssbo_production_index_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssbo_offset_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, ssbo_new_productions__ignored);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, ssbo_previous_look_back_buffer);
            glDispatchCompute(result_buffer_size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            this_find_production_and_size_program->unuse();

            // run prefix sum on the copy
            run_prefix_sum(ssbo_offset_buffer, result_buffer_size);

            // get buffer element at the end that will be the size of the result buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_offset_buffer);
            // here we change the stored value to the size of the result buffer
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (result_buffer_size - 1) * sizeof(uint32_t), sizeof(uint32_t),
                               &result_buffer_size);

            // generate productions for each letter into next result buffer
            this_production_shader_program->use();

            // make space for next result buffer
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_look_back_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, result_buffer_size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_new_productions__successors_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_new_productions__successors_sizes);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_new_productions__successors_data);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_previous_result_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_offset_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_next_result_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_new_productions__look_back);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_previous_look_back_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_next_look_back_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_production_index_buffer);

            glDispatchCompute(result_buffer_size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            this_production_shader_program->unuse();

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
        }

        ssbo_word_length = result_buffer_size;
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

    template<typename T>
    void print_ssbo(GLuint ssbo, size_t elem_count, const std::string name,
                    std::function<void(T)> printer = [](T t) { std::cout << t; }) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        auto *data = (T *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        for (size_t i = 0; i < elem_count; i++) {
            std::cout << name << "[" << i << "] = ";
            printer(data[i]);
            std::cout << std::endl;
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
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
        this_leaf_edge_detector_program->setUniform("alternative_char_to_find", (int) '{');
        this_leaf_edge_detector_program->setUniform("second_alternative_char_to_find", (int) '}');
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

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_leaf_edge_counter);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (size - 1) * sizeof(int32_t), sizeof(int32_t), &leaf_edge_count);

        // reuse ssbo_next_result_buffer for counting
        // resize buffer to the size of the word
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_next_result_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

        this_instance_detector_program->use();
        this_instance_detector_program->setUniform("markLeaf", MARK_LEAF);
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
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, (size - 1) * sizeof(uint32_t), sizeof(uint32_t),
                           &drawable_instances_count);

//        std::cout << "Drawable instances count: " << drawable_instances_count << std::endl;

        // BEGIN: color and length update
        prefix_job_on_tree_program->use();

        // init input jumps to be LookBack table
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input_jumps);
        // copy ssbo_previous_look_back_buffer into ssbo_input_jumps
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);
        glCopyNamedBufferSubData(ssbo_previous_look_back_buffer, ssbo_input_jumps, 0, 0, size * sizeof(int32_t));

        // init output jumps to be LookBack table
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_output_jumps);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);

        // copy contents of ssbo to ssbo_colors_buffer_input (ssbo_colors_buffer_input holds ints)
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_color_buffer_input);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);
        glCopyNamedBufferSubData(ssbo, ssbo_color_buffer_input, 0, 0, size * sizeof(int32_t));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_color_buffer_output);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_length_buffer_input);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(float), nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_length_buffer_output);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(float), nullptr, GL_STATIC_DRAW);

        prefix_job_on_tree_program->setUniform("number_of_colors", (int) grammar->get_colors().size());
        prefix_job_on_tree_program->setUniform("factor", grammar->get_property_float("factor", 0.8f));

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_color_buffer_input);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_length_buffer_input);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_input_jumps);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_color_buffer_output);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_length_buffer_output);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_output_jumps);

        auto epochs = (size_t) ceil(log2((double) size));
        for (size_t epoch = 0; epoch <= epochs; ++epoch) {
            prefix_job_on_tree_program->setUniform("epoch", (int) epoch);
            glDispatchCompute(size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            glCopyNamedBufferSubData(ssbo_output_jumps, ssbo_input_jumps, 0, 0, size * sizeof(int32_t));
            glCopyNamedBufferSubData(ssbo_color_buffer_output, ssbo_color_buffer_input, 0, 0,
                                     size * sizeof(int32_t));
            glCopyNamedBufferSubData(ssbo_length_buffer_output, ssbo_length_buffer_input, 0, 0,
                                     size * sizeof(float));
        }

        prefix_job_on_tree_program->unuse();
        // END: color and length update

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
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_length_buffer_input);

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
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo);

        // overestimate epochs to be ceil of log2 of size
        // no more epochs needed than all nodes in the tree
//        auto epochs = (size_t) ceil(log2((double) size));
        for (size_t epoch = 0; epoch <= epochs; ++epoch) {

            // run the compute
            this_matrix_multiplier_program->setUniform("epoch", (int) epoch);
            glDispatchCompute(size, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // copy output into output for both jumps and transformations
            glCopyNamedBufferSubData(ssbo_output_jumps, ssbo_input_jumps, 0, 0, size * sizeof(int32_t));
            glCopyNamedBufferSubData(transformations_output_ssbo, transformations_input_ssbo, 0, 0,
                                     size * sizeof(glm::mat4));
        }
        this_matrix_multiplier_program->unuse();

        instance_translations_count = drawable_instances_count;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_translations);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instance_translations_count * sizeof(glm::mat4),
                     nullptr, GL_STATIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_colors__index);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instance_translations_count * sizeof(uint32_t),
                     nullptr, GL_STATIC_DRAW);

        // place instances
        this_instance_placer_program->use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, transformations_input_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_translations);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_next_result_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_length_buffer_input);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_color_buffer_input);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_colors__index);

        glm::mat4 common_turtle_matrix = glm::transpose(glm::mat4{
                0, 1, 0, 0,
                -1, 0, 0, 0,
                0, 0, 1, 0,
                0, -downset, 0, 1
        });

        auto move_down = glm::translate(glm::mat4(1.0), glm::vec3(-0.5f, 0.0, 0.0f));
        auto move_back_up = glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.0f, 0.0f));

        this_instance_placer_program->setUniform("common_turtle_matrix", common_turtle_matrix);
        this_instance_placer_program->setUniform("common_cube_matrix_a", move_down);
        this_instance_placer_program->setUniform("common_cube_matrix_c", move_back_up);
        this_instance_placer_program->setUniform("step", step);

        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_instance_placer_program->unuse();

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_leaf_positions_vec4);
        glBufferData(GL_SHADER_STORAGE_BUFFER, leaf_edge_count * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_leaf_color_int);
        glBufferData(GL_SHADER_STORAGE_BUFFER, leaf_edge_count * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_leaf_index_array);
        glBufferData(GL_SHADER_STORAGE_BUFFER, leaf_edge_count * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        this_leaf_position_placer_program->use();
        this_leaf_position_placer_program->setUniform("char_of_leaf_edge", (int) 'f');
        this_leaf_position_placer_program->setUniform("alternative_char_of_leaf_edge", (int) '{');
        this_leaf_position_placer_program->setUniform("second_alternative_char_of_leaf_edge", (int) '}');
        this_leaf_position_placer_program->setUniform("common_turtle_matrix", common_turtle_matrix);
        this_leaf_position_placer_program->setUniform("reset_index_value", (uint32_t) -1);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_leaf_edge_counter);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, transformations_input_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_leaf_positions_vec4);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_leaf_index_array);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_is_a_leaf_output);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_leaf_color_int);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_color_buffer_input);

        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        this_leaf_position_placer_program->unuse();

        // ssbo_colors__index
    }

    void run_leaf_draw(const glm::mat4 &pvm, const glm::vec4 &eye_pos) {
        // draw GL_TRIANGLE_FAN for vertices in ssbo_leaf_positions_vec4, and indecision in ssbo_leaf_index_array with reset index enabled

        this_leaf_program->use();

        glEnable(GL_PRIMITIVE_RESTART);
        glPrimitiveRestartIndex((uint32_t) -1);

        glBindVertexArray(vao_leaf);
        glBindBuffer(GL_ARRAY_BUFFER, ssbo_leaf_positions_vec4);
        this_leaf_program->setAttribute("position", 4, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, ssbo_leaf_color_int);
        this_leaf_program->setAttribute("colorId", 1, 0, 0, GL_FALSE, GL_INT);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_colors__values);

        set_light_and_pv(this_leaf_program, pvm, eye_pos);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ssbo_leaf_index_array);
        glDrawElements(GL_TRIANGLE_FAN, leaf_edge_count, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);

        glDisable(GL_PRIMITIVE_RESTART);

        this_leaf_program->unuse();
    }

    GrammarPtr grammar;

    GLuint vao{};
    GLuint vbo_point{};
    GLuint ibo{};
    GLuint ssbo_translations{};
    GLuint ssbo_colors__values{};
    GLuint ssbo_colors__index{};
    GLuint instance_translations_count = -1;

    GLuint ssbo_color_buffer_input{};
    GLuint ssbo_color_buffer_output{};
    GLuint ssbo_length_buffer_input{};
    GLuint ssbo_length_buffer_output{};

    float downset{};

    size_t ssbo_word_length{};
    size_t productions_count{};
    size_t ignored_count{};
    int32_t leaf_edge_count{};

    GLuint vao_leaf{};

    GLuint ssbo_new_productions__predecessors{};
    GLuint ssbo_new_productions__look_back{};
    GLuint ssbo_new_productions__successors_sizes{};
    GLuint ssbo_new_productions__successors_offsets{};
    GLuint ssbo_new_productions__successors_data{};
    GLuint ssbo_new_productions__left_context_sizes{};
    GLuint ssbo_new_productions__left_context_offsets{};
    GLuint ssbo_new_productions__left_context_data{};
    GLuint ssbo_new_productions__right_context_sizes{};
    GLuint ssbo_new_productions__right_context_offsets{};
    GLuint ssbo_new_productions__right_context_data{};
    GLuint ssbo_new_productions__ignored{};

    GLuint ssbo_previous_result_buffer{};
    GLuint ssbo_offset_buffer{};
    GLuint ssbo_production_index_buffer{};
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
    GLuint ssbo_leaf_positions_vec4{};
    GLuint ssbo_leaf_color_int{};
    GLuint ssbo_leaf_index_array{};

    std::shared_ptr<ShaderProgram> this_production_shader_program;
    std::shared_ptr<ShaderProgram> this_prefix_sum_shader_program;
    std::shared_ptr<ShaderProgram> this_instance_detector_program;
    std::shared_ptr<ShaderProgram> this_matrix_filler_program;
    std::shared_ptr<ShaderProgram> this_matrix_multiplier_program;
    std::shared_ptr<ShaderProgram> this_instance_placer_program;
    std::shared_ptr<ShaderProgram> this_leaf_detector_program;
    std::shared_ptr<ShaderProgram> this_leaf_edge_detector_program;
    std::shared_ptr<ShaderProgram> this_leaf_begin_detector_program;
    std::shared_ptr<ShaderProgram> this_leaf_position_placer_program;
    std::shared_ptr<ShaderProgram> this_leaf_program;
    std::shared_ptr<ShaderProgram> this_find_production_and_size_program;
    std::shared_ptr<ShaderProgram> this_prefix_job_on_tree_program;

    static std::shared_ptr<ShaderProgram> system_shader_program;
    static std::shared_ptr<ShaderProgram> production_shader_program;
    static std::shared_ptr<ShaderProgram> prefix_sum_shader_program;
    static std::shared_ptr<ShaderProgram> instance_detector_program;
    static std::shared_ptr<ShaderProgram> matrix_filler_program;
    static std::shared_ptr<ShaderProgram> matrix_multiplier_program;
    static std::shared_ptr<ShaderProgram> instance_placer_program;
    static std::shared_ptr<ShaderProgram> leaf_detector_program;
    static std::shared_ptr<ShaderProgram> leaf_edge_detector_program;
    static std::shared_ptr<ShaderProgram> leaf_begin_detector_program;
    static std::shared_ptr<ShaderProgram> leaf_position_placer_program;
    static std::shared_ptr<ShaderProgram> leaf_program;
    static std::shared_ptr<ShaderProgram> find_production_and_size_program;
    static std::shared_ptr<ShaderProgram> prefix_job_on_tree_program;
};

std::shared_ptr<ShaderProgram> SystemDrawable::system_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::production_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::prefix_sum_shader_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::instance_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::matrix_filler_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::matrix_multiplier_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::instance_placer_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_edge_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_begin_detector_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_position_placer_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::leaf_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::find_production_and_size_program = nullptr;
std::shared_ptr<ShaderProgram> SystemDrawable::prefix_job_on_tree_program = nullptr;

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
