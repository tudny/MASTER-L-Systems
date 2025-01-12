#include <GLFW/glfw3.h>
#include <cmath>
#include "View.h"
#include "glm/detail/type_vec4.hpp"
#include "glm/ext/matrix_transform.hpp"

glm::mat4 RotateView::get_view_matrix() const {
    return glm::lookAt(
            glm::vec3(this->get_eye_pos()),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
    );
}

glm::vec4 RotateView::get_eye_pos() const {
    double time = glfwGetTime() * this->speed * this->direction * this->enabled;
    return {
            this->distance * sin(time),
            this->distance * cos(time),
            this->height, 1.0
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
