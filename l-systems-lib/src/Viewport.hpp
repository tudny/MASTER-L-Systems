#ifndef LSYSTEMS_VIEWPORT_HPP
#define LSYSTEMS_VIEWPORT_HPP


#include <glm/glm.hpp>

class Viewport {
    int left, top, width, height, window_width, window_height;

    glm::vec2 window_to_local_fixed_ratio(double x, double y) const;
    glm::vec2 window_to_local_stretch(double x, double y) const;

    glm::mat3 local_fixed_ratio_to_standard_square() const;
    glm::mat3 local_stretch_to_standard_square() const;
};


#endif //LSYSTEMS_VIEWPORT_HPP
