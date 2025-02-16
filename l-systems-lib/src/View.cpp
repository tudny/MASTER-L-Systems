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

glm::mat4 RotateView::get_view_matrix() const {
    return glm::lookAt(
            glm::vec3(this->get_eye_pos()),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

glm::vec4 RotateView::get_eye_pos() const {
    double time = glfwGetTime() * this->speed * as_integer(this->direction) * this->enabled;
    return {
            this->distance * sin(time),
            this->height,
            this->distance * cos(time),
            1.,
    };
}

RotateView::RotateView(RotateView::Direction direction, float speed, float distance, float height) : direction(
        direction), speed(speed), distance(distance), height(height) {}

void RotateView::enable() {
    this->enabled = true;
}

void RotateView::disable() {
    this->enabled = false;
}

void RotateView::switch_on_off() {
    this->enabled = !this->enabled;
}

void RotateView::zoom(float offset) {
    this->distance += offset * ZOOM_FACTOR;
}
