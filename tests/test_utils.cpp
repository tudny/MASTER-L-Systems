#include "test_utils.hpp"

void run_opengl_test(const std::function<void()> &init, const std::function<void()> &task) {
    TextContext context{init, task};
}
