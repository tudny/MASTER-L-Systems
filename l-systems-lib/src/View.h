#ifndef LSYSTEMS_VIEW_H
#define LSYSTEMS_VIEW_H

#include <functional>
#include "glm/fwd.hpp"

class View {
public:
    [[nodiscard]] virtual glm::mat4 get_view_matrix() const = 0;

    [[nodiscard]] virtual glm::vec4 get_eye_pos() const = 0;

    virtual ~View() = default;
};

/**
 * @brief Rotate view class
 *
 * Rotate view class is a view that rotates around the object at (0, 0, 0).
 * It can rotate left or right.
 * It rotates around z-axis around (0, 0, 0) with given speed at given distance at given height.
 */
class RotateView : public View {
public:
    static constexpr float ZOOM_FACTOR = 0.1f;

    enum Direction {
        CLOCKWISE = -1,
        COUNTER_CLOCKWISE = 1,
    };

    RotateView(Direction direction, float speed, float distance, float height);

    [[nodiscard]] glm::mat4 get_view_matrix() const override;

    [[nodiscard]] glm::vec4 get_eye_pos() const override;

    void enable();

    void disable();

    void switch_on_off();

    ~RotateView() override = default;

    void zoom(float);

private:
    Direction direction;
    float speed;
    float distance;
    float height;
    bool enabled = true;
};

#endif //LSYSTEMS_VIEW_H
