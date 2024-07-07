#ifndef LSYSTEMS_DRAWABLE_HPP
#define LSYSTEMS_DRAWABLE_HPP


#include "Viewport.hpp"

class Drawable {
public:
    explicit Drawable(Viewport viewport) : viewport(viewport) {}

    virtual ~Drawable() = default;

    virtual void draw() = 0;

    virtual void update_viewport(Viewport _viewport);

    [[nodiscard]] Viewport get_viewport() const;

    virtual void on_mouse_button(
            [[maybe_unused]] int button,
            [[maybe_unused]] int action,
            [[maybe_unused]] int mods,
            [[maybe_unused]] double x,
            [[maybe_unused]] double y
    ) {}

    virtual void on_cursor_position(
            [[maybe_unused]] double x,
            [[maybe_unused]] double y
    ) {}

protected:
    Viewport viewport;
};


#endif //LSYSTEMS_DRAWABLE_HPP
