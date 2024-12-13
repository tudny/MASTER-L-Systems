#include "Drawable.hpp"

#include <utility>

void Drawable::update_viewport(Viewport _viewport) {
    this->viewport = _viewport;
}

Viewport Drawable::get_viewport() const {
    return viewport;
}

void Drawable::update_view(std::shared_ptr<View> _view) {
    this->view = std::move(_view);
}

std::shared_ptr<View> Drawable::get_view() const {
    return this->view;
}
