#include "lib.hpp"

std::string greeter() {
    return "---------------------------------------------------------\n" \
        "Welcome to the L-System application!\n" \
        "This application is designed to work with L-Systems and their grammars.\n" \
        "It provides a graphical interface for visualizing L-Systems.\n" \
        "Please refer to the documentation for more details.\n" \
        "---------------------------------------------------------\n";
}

void global_run_prefix_sum(std::shared_ptr<ShaderProgram> &shader, GLuint ssbo, size_t size) {
    shader->use();

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
        shader->setUniform("step", (int) step);
        glDispatchCompute(size, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // put output back into input
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glCopyNamedBufferSubData(ssbo_output, ssbo, 0, 0, size * sizeof(uint32_t));
    }

    glDeleteBuffers(1, &ssbo_output);

    shader->unuse();
}
