/** @file
 * @brief Greeter function declaration
 *
 * This file contains the declaration of the greeter function.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 29.06.2024
*/
#include <string>
#include <utility>
#include <iostream>
#include "shader.hpp"
#include "memory"
#include "errors.hpp"

/**
 * @brief Greeter function
 *
 * Greeter function returns a string "Hello, World!"
 *
 * @return std::string "Hello, World!"
 */
std::string greeter();

// Exported for testing purposes
void global_run_prefix_sum(std::shared_ptr<ShaderProgram> &shader, GLuint ssbo, size_t size);


class Measure {
public:

    GLuint time_elapsed_query{};
    GLint time_elapsed_query_available{};
    GLuint time_elapsed_query_result{};

    std::string label{};
    std::function<void()> action{};

    Measure(std::string _label) : label(std::move(_label)) {
        glGenQueries(1, &time_elapsed_query);
    }

    void init(std::function<void()> _action) {
        this->action = std::move(_action);
    }

    void print() {
        uint64_t result = run();
        std::cout << "Task [" << label << "] took " << result << " ms" << std::endl;
    }

    uint64_t run() {
        glBeginQuery(GL_TIME_ELAPSED, time_elapsed_query);
        GL_CHECK_ERROR();
        action();
        glEndQuery(GL_TIME_ELAPSED);
        GL_CHECK_ERROR();
        time_elapsed_query_available = 0;
        while (!time_elapsed_query_available) {
            glGetQueryObjectiv(time_elapsed_query, GL_QUERY_RESULT_AVAILABLE, &time_elapsed_query_available);
            GL_CHECK_ERROR();
        }
        time_elapsed_query_result = 0;
        glGetQueryObjectuiv(time_elapsed_query, GL_QUERY_RESULT, &time_elapsed_query_result);
        GL_CHECK_ERROR();

        return time_elapsed_query_result / 1000000;
    }
};
