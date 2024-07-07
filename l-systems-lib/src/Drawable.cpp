#include "Drawable.hpp"

void Drawable::update_viewport(Viewport _viewport) {
    this->viewport = _viewport;
}

Viewport Drawable::get_viewport() const {
    return viewport;
}
