#include <GLFW/glfw3.h>
#include <cmath>
#include "View.h"
#include "glm/detail/type_vec4.hpp"
#include "glm/ext/matrix_transform.hpp"

// Taken from https://stackoverflow.com/a/11421471/7095554
template<typename E>
auto as_integer(E const value)
-> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(value);
}

glm::mat4 RotateView::get_view_matrix() {
    return glm::lookAt(
            glm::vec3(this->get_eye_pos()),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

glm::vec4 RotateView::get_eye_pos() {
    auto current_time = static_cast<float>(glfwGetTime());
    float delta_time = current_time - this->last_update;
    if (this->enabled) {
        this->rotation_state += this->speed * static_cast<float>(as_integer(this->direction)) * delta_time;
        this->last_update = current_time;
    }

    return {
            this->distance * sinf(this->rotation_state),
            this->height,
            this->distance * cosf(this->rotation_state),
            1.,
    };
}

RotateView::RotateView(RotateView::Direction direction, float speed, float distance, float height) : direction(
        direction), speed(speed), distance(distance), height(height) {}

void RotateView::switch_on_off() {
    this->enabled = !this->enabled;

    if (this->enabled) {
        this->last_update = static_cast<float>(glfwGetTime());
    }
}

void RotateView::zoom(float offset) {
    this->distance += offset * ZOOM_FACTOR;
}
