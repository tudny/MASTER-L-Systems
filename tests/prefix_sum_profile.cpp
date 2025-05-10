#include "test_utils.hpp"
#include "lib.hpp"

void fill_ssbo(GLuint ssbo, size_t size) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(int32_t), nullptr, GL_STATIC_DRAW);
    auto data = (int32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    for (size_t i = 0; i < size; ++i) {
        data[i] = 1;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void run_prefix_sum_on_cpu(size_t size) {
    std::vector<int32_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = 1;
    }

    float start = glfwGetTime();
    for (size_t i = 1; i < size; ++i) {
        data[i] += data[i - 1];
    }
    float end = glfwGetTime();

    std::cout << "Time taken for CPU prefix sum: " << (end - start) * 1000.0 << " ms" << std::endl;
}

void check_ssbo(GLuint ssbo, size_t size) {
    std::vector<int32_t> expected_data(size);
    for (size_t i = 0; i < size; ++i) {
        expected_data[i] = static_cast<int32_t>(i) + 1;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    auto data = (int32_t *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (size_t i = 0; i < size; ++i) {
        EXPECT_EQ(data[i], expected_data[i]) << "Mismatch at index " << i;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

class PrefixSumProfile : public ::testing::TestWithParam<size_t> {
};

TEST_P(PrefixSumProfile, PrefixSumProfile) {
    std::shared_ptr<ShaderProgram> find_production_and_size_shader_program = nullptr;

    GLuint ssbo;
    size_t size = GetParam();

    run_opengl_test([&]() {
        find_production_and_size_shader_program = std::make_shared<ShaderProgram>(
                std::initializer_list<Shader>{Shader{
                        FIXTURE_PATH("resources/shaders/productions/prefix_sum.comp"),
                        GL_COMPUTE_SHADER
                }}
        );

        glGenBuffers(1, &ssbo);
        fill_ssbo(ssbo, size);
    }, [&]() {
        double start = glfwGetTime();
        global_run_prefix_sum(find_production_and_size_shader_program, ssbo, size);
        double end = glfwGetTime();

        std::cout << "Time taken for prefix sum: " << (end - start) * 1000.0 << " ms" << std::endl;

        check_ssbo(ssbo, size);

        glDeleteBuffers(1, &ssbo);

        run_prefix_sum_on_cpu(size);
    });
}

INSTANTIATE_TEST_SUITE_P(
        PrefixSumProfileTests,
        PrefixSumProfile,
        ::testing::Values(1024, 1024 * 16, 1024 * 1024, 1024 * 1024 * 16)
);
