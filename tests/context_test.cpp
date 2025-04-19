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
) {
    TextContext context{init, task};
}

struct SSBONewProductions {
    GLuint predecessors{};
    GLuint look_back{};
    GLuint successors_sizes{};
    GLuint successors_offsets{};
    GLuint successors_data{};
    GLuint left_context_sizes{};
    GLuint left_context_offsets{};
    GLuint left_context_data{};
    GLuint right_context_sizes{};
    GLuint right_context_offsets{};
    GLuint right_context_data{};
    GLuint ignored{};

    int32_t productions_count{};
    int32_t ignored_count{};
};

void preparse_productions(const GrammarPtr &grammar, SSBONewProductions &ssbo_new_productions) {
    auto productions_gl_data = grammar->get_opengl_ready_productions();
    std::vector<std::pair<GLuint *, OpenGLReadyProductionDataType>> mappings = {
            {&ssbo_new_productions.predecessors,          productions_gl_data.predecessors},
            {&ssbo_new_productions.look_back,             productions_gl_data.look_back},
            {&ssbo_new_productions.successors_sizes,      productions_gl_data.successors_sizes},
            {&ssbo_new_productions.successors_offsets,    productions_gl_data.successors_offsets},
            {&ssbo_new_productions.successors_data,       productions_gl_data.successors_data},
            {&ssbo_new_productions.left_context_sizes,    productions_gl_data.left_context_sizes},
            {&ssbo_new_productions.left_context_offsets,  productions_gl_data.left_context_offsets},
            {&ssbo_new_productions.left_context_data,     productions_gl_data.left_context_data},
            {&ssbo_new_productions.right_context_sizes,   productions_gl_data.right_context_sizes},
            {&ssbo_new_productions.right_context_offsets, productions_gl_data.right_context_offsets},
            {&ssbo_new_productions.right_context_data,    productions_gl_data.right_context_data},
            {&ssbo_new_productions.ignored,               productions_gl_data.ignored}
    };
    ssbo_new_productions.productions_count = static_cast<int32_t>(grammar->get_raw_productions().size());
    ssbo_new_productions.ignored_count = static_cast<int32_t>(grammar->get_ignored()->ignored.size());

    for (const auto &[buffer, data]: mappings) {
        glGenBuffers(1, buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, *buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, data->size() * sizeof(int32_t),
                     data->data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void bind_shaders(SSBONewProductions &ssbo_new_productions) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_new_productions.predecessors);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_new_productions.look_back);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_new_productions.successors_sizes);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_new_productions.successors_offsets);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_new_productions.successors_data);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_new_productions.left_context_sizes);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_new_productions.left_context_offsets);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_new_productions.left_context_data);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_new_productions.right_context_sizes);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_new_productions.right_context_offsets);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_new_productions.right_context_data);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, ssbo_new_productions.ignored);
}

struct InputOutputBuffers {
    GLuint word_input_buffer{};
    GLuint word_lookback_input_buffer{};
    GLuint production_index_output_buffer{};
    GLuint production_size_output_buffer{};
};

void init_InputOutputBuffers(InputOutputBuffers &inputOutputBuffers, const std::string &word) {
    auto look_back = compute_look_back(word);
    Axiom axiom{word, look_back};
    auto word_data = axiom.get_as_opengl_data();
    glGenBuffers(1, &inputOutputBuffers.word_input_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputOutputBuffers.word_input_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, word_data.size() * sizeof(int32_t),
                 word_data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glGenBuffers(1, &inputOutputBuffers.word_lookback_input_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputOutputBuffers.word_lookback_input_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, look_back.size() * sizeof(int32_t),
                 look_back.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glGenBuffers(1, &inputOutputBuffers.production_index_output_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputOutputBuffers.production_index_output_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, word_data.size() * sizeof(int32_t),
                 nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glGenBuffers(1, &inputOutputBuffers.production_size_output_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputOutputBuffers.production_size_output_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, word_data.size() * sizeof(int32_t),
                 nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void dispose_InputOutputBuffers(InputOutputBuffers &inputOutputBuffers) {
    glDeleteBuffers(1, &inputOutputBuffers.word_input_buffer);
    glDeleteBuffers(1, &inputOutputBuffers.word_lookback_input_buffer);
    glDeleteBuffers(1, &inputOutputBuffers.production_index_output_buffer);
    glDeleteBuffers(1, &inputOutputBuffers.production_size_output_buffer);
}

void bind_InputOutputBuffers(InputOutputBuffers &inputOutputBuffers) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, inputOutputBuffers.word_input_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, inputOutputBuffers.production_index_output_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, inputOutputBuffers.production_size_output_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, inputOutputBuffers.word_lookback_input_buffer);
}

void set_uniforms(
        const std::shared_ptr<ShaderProgram> &find_production_and_size_shader_program,
        const int &word_size,
        const int &productions_count,
        const int &ignored_count
) {
    find_production_and_size_shader_program->setUniform("wholeInputSize", word_size);
    find_production_and_size_shader_program->setUniform("numberOfProductions", productions_count);
    find_production_and_size_shader_program->setUniform("ignoredSize", ignored_count);
}

template<typename T>
std::vector<T> pull_data(GLuint buffer, size_t elements) {
    std::vector<T> data(elements);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, elements * sizeof(T), data.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return data;
}

struct ContextTestResult {
    std::vector<int32_t> production_indices;
    std::vector<int32_t> production_sizes;
};

ContextTestResult run_context_word_test(
        const GrammarPtr &grammar,
        const std::string &word
) {
    std::shared_ptr<ShaderProgram> find_production_and_size_shader_program = nullptr;

    SSBONewProductions ssbo_new_productions;
    InputOutputBuffers inputOutputBuffers;

    ContextTestResult contextTestResult;

    run_opengl_test(
            [&]() {
                find_production_and_size_shader_program = std::make_shared<ShaderProgram>(
                        std::initializer_list<Shader>{Shader{
                                FIXTURE_PATH("resources/shaders/productions/find_production_and_size.comp"),
                                GL_COMPUTE_SHADER
                        }}
                );
                preparse_productions(grammar, ssbo_new_productions);
            },
            [&]() {
                auto word_length = word.size();

                init_InputOutputBuffers(inputOutputBuffers, word);

                find_production_and_size_shader_program->use();
                set_uniforms(
                        find_production_and_size_shader_program,
                        static_cast<int>(word_length),
                        ssbo_new_productions.productions_count,
                        ssbo_new_productions.ignored_count
                );
                bind_shaders(ssbo_new_productions);
                bind_InputOutputBuffers(inputOutputBuffers);
                glDispatchCompute(word_length, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                find_production_and_size_shader_program->unuse();

                contextTestResult.production_indices = pull_data<int32_t>(
                        inputOutputBuffers.production_index_output_buffer,
                        word_length
                );

                contextTestResult.production_sizes = pull_data<int32_t>(
                        inputOutputBuffers.production_size_output_buffer,
                        word_length
                );

                dispose_InputOutputBuffers(inputOutputBuffers);
            }
    );

    return contextTestResult;
}

ContextTestResult run_context_word_test(
        const std::string &grammar_path,
        const std::string &word
) {
    auto grammar = load_grammar(FIXTURE_PATH("resources/" + grammar_path));
    return run_context_word_test(grammar, word);
}

TEST(Context, CheckContextShader1) {
    auto results = run_context_word_test("context_a1.ls", "F1F1F1");

    std::vector<int32_t> expected_production_indices = {-1, -1, -1, 7, -1, -1};
    EXPECT_EQ(results.production_indices, expected_production_indices);

    std::vector<int32_t> expected_production_sizes = {1, 1, 1, 1, 1, 1};
    EXPECT_EQ(results.production_sizes, expected_production_sizes);
}

TEST(Context, CheckContextShader2) {
    auto results = run_context_word_test("context_a1.ls", "F1F0F1");

    std::vector<int32_t> expected_production_indices = {-1, -1, -1, 5, -1, -1};
    EXPECT_EQ(results.production_indices, expected_production_indices);

    std::vector<int32_t> expected_production_sizes = {1, 1, 1, 3, 1, 1};
    EXPECT_EQ(results.production_sizes, expected_production_sizes);
}

TEST(Context, CheckContextShader3) {
    auto results = run_context_word_test("context_a1.ls", "F1F1F1F1");

    std::vector<int32_t> expected_production_indices = {-1, -1, -1, 7, -1, 7, -1, -1};
    EXPECT_EQ(results.production_indices, expected_production_indices);

    std::vector<int32_t> expected_production_sizes = {1, 1, 1, 1, 1, 1, 1, 1};
    EXPECT_EQ(results.production_sizes, expected_production_sizes);
}

struct SingleMatch {
    int32_t production_index;
    int32_t production_size;
    std::string successor;
};

bool are_equal(const char &a, const char &c) {
    if (c == '*') {
        return true;
    }
    return a == c;
}

bool do_match_left_context(
        const std::string &left_slice,
        const std::string &left_context,
        const std::vector<char> &ignored_chars
) {
    int word_index = static_cast<int>(left_slice.size()) - 1;
    int context_index = static_cast<int>(left_context.size()) - 1;

    auto find_matching_bracket = [&](const int current_index) {
        int state = 0;
        int idx = current_index - 1;
        while (idx >= 0) {
            if (left_slice[idx] == ']') {
                state++;
            } else if (left_slice[idx] == '[') {
                if (state == 0) {
                    return idx;
                } else {
                    state--;
                }
            }
            idx--;
        }
        throw std::runtime_error(
                "Unbalanced brackets in left context: " + left_slice + " at index " + std::to_string(current_index));
    };

    while (word_index >= 0 && context_index >= 0) {
        if (std::find(ignored_chars.begin(), ignored_chars.end(), left_slice[word_index]) !=
            ignored_chars.end()) {
            word_index--;
        } else if (are_equal(left_slice[word_index], left_context[context_index])) {
            word_index--;
            context_index--;
        } else if (left_slice[word_index] == ']') {
            word_index = find_matching_bracket(word_index);
        } else if (left_slice[word_index] == '[') {
            word_index--;
        } else {
            return false;
        }
    }
    return context_index < 0;
}

bool do_match_right_context(
        const std::string &right_slice,
        const std::string &right_context,
        const std::vector<char> &ignored_chars
) {
    int word_index = 0;
    int context_index = 0;

    auto find_matching_bracket = [&](const int current_index) {
        int state = 0;
        int idx = current_index + 1;
        while (idx < static_cast<int>(right_slice.size())) {
            if (right_slice[idx] == '[') {
                state++;
            } else if (right_slice[idx] == ']') {
                if (state == 0) {
                    return idx;
                } else {
                    state--;
                }
            }
            idx++;
        }
        throw std::runtime_error(
                "Unbalanced brackets in right context: " + right_slice + " at index " + std::to_string(current_index));
    };

    while (word_index < static_cast<int>(right_slice.size()) &&
           context_index < static_cast<int>(right_context.size())) {
        if (std::find(ignored_chars.begin(), ignored_chars.end(), right_slice[word_index]) !=
            ignored_chars.end()) {
            word_index++;
        } else if (are_equal(right_slice[word_index], right_context[context_index])) {
            word_index++;
            context_index++;
        } else if (right_slice[word_index] == '[') {
            word_index = find_matching_bracket(word_index);
        } else if (right_slice[word_index] == ']') {
            word_index++;
        } else {
            return false;
        }
    }
    return context_index >= static_cast<int>(right_context.size());
}

SingleMatch match_production(const GrammarPtr &grammar, const std::string &word, int32_t index) {
    std::string left_slice = word.substr(0, index);
    std::string right_slice = word.substr(index + 1);

    std::vector<int> ignored_chars_ints = grammar->get_ignored()->ignored;
    std::vector<char> ignored_chars(ignored_chars_ints.begin(), ignored_chars_ints.end());

    char letter = word[index];
    auto productions = grammar->get_raw_productions();

    for (size_t i = 0; i < productions.size(); i++) {
        const auto &prod = productions[i];
        if (prod.predecessor != letter) {
            continue;
        }
        if (!do_match_left_context(left_slice, prod.left_context, ignored_chars)) {
            continue;
        }
        if (!do_match_right_context(right_slice, prod.right_context, ignored_chars)) {
            continue;
        }
        std::string successor = prod.successor;

        return {
                .production_index = static_cast<int32_t>(i),
                .production_size = static_cast<int32_t>(successor.size()),
                .successor = successor
        };
    }
    return {
            .production_index = -1,
            .production_size = 1,
            .successor = std::string{letter}
    };
}

struct MultiMatch {
    std::vector<int32_t> production_indices;
    std::vector<int32_t> production_sizes;
    std::string successor;
};

MultiMatch match_production_multi(const GrammarPtr &grammar, const std::string &word) {
    std::vector<int32_t> production_indices;
    std::vector<int32_t> production_sizes;
    std::string successor;

    for (size_t i = 0; i < word.size(); i++) {
        auto match = match_production(grammar, word, static_cast<int32_t>(i));
        production_indices.push_back(match.production_index);
        production_sizes.push_back(match.production_size);
        successor += match.successor;
    }
    return {
            .production_indices = production_indices,
            .production_sizes = production_sizes,
            .successor = successor
    };
}

TEST(Context, CheckContextShaderBatch) {
    const std::string grammar_path = "context_a1.ls";
    const size_t batch_size = 30;

    auto grammar = load_grammar(FIXTURE_PATH("resources/" + grammar_path));
    auto productions = grammar->get_raw_productions();

    std::string word = "F1F1F1";

    for (size_t batch = 0; batch < batch_size; batch++) {
        std::cout << "Batch " << batch << ": " << word.size() << std::endl;

        auto expected_result = match_production_multi(grammar, word);
        auto results = run_context_word_test(grammar, word);

        std::stringstream error_message;
        error_message << "Batch " << batch << ": ";
        error_message << "Word: " << word << ", ";
        error_message << "Size: " << word.size() << ", ";

        ASSERT_EQ(results.production_indices, expected_result.production_indices) << error_message.str();
        ASSERT_EQ(results.production_sizes, expected_result.production_sizes) << error_message.str();

        word = expected_result.successor;
    }
}
