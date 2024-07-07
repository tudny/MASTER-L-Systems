#include "Viewport.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>

#define CAST(x) static_cast<float>(x)

glm::vec2 Viewport::window_to_local_fixed_ratio(double x, double y) const {
    double d = std::min(width, height);
    return {
            (x - (left + width / 2.)) / d * 2.,
            (top + height / 2. - y) / d * 2.
    };
}

glm::vec2 Viewport::window_to_local_stretch(double x, double y) const {
    return {
            (x - left) / width * 2. - 1.,
            1. - (y - top) / height * 2.
    };
}

glm::mat3 Viewport::local_fixed_ratio_to_standard_square() const {
    auto ww = CAST(window_width);
    auto wh = CAST(window_height);
    auto d = std::min(CAST(width), CAST(height));
    auto mid_x = (2.f * CAST(left) + CAST(width)) / ww - 1.f;
    auto mid_y = 1.f - (2.f * CAST(top) + CAST(height)) / wh;

    return glm::scale(
            glm::translate(glm::mat3(1), glm::vec2(mid_x, mid_y)),
            glm::vec2(d / ww, d / wh)
    );
}

glm::mat3 Viewport::local_stretch_to_standard_square() const {
    auto ww = CAST(window_width);
    auto wh = CAST(window_height);
    auto mid_x = (2.f * CAST(left) + CAST(width)) / ww - 1.f;
    auto mid_y = 1.f - (2.f * CAST(top) + CAST(height)) / wh;

    return glm::scale(
            glm::translate(glm::mat3(1.f), glm::vec2(mid_x, mid_y)),
            glm::vec2(CAST(width) / ww, CAST(height) / wh)
    );
}
