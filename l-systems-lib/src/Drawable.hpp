/** @file
 * @brief Drawable class declaration
 *
 * This file contains the declaration of the Drawable class.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 07.07.2024
*/

#ifndef LSYSTEMS_DRAWABLE_HPP
#define LSYSTEMS_DRAWABLE_HPP


#include "Viewport.hpp"

/// Drawable class abstracts the drawable object
class Drawable {
public:
    /**
     * @brief Drawable constructor
     *
     * Drawable constructor creates a drawable object with given viewport.
     *
     * @param viewport Viewport
     */
    explicit Drawable(Viewport viewport) : viewport(viewport) {}

    /**
     * @brief Drawable destructor
     *
     * Drawable destructor is a default destructor.
     */
    virtual ~Drawable() = default;

    /**
     * @brief Draw the object
     *
     * Draw the object on the screen.
     */
    virtual void draw() = 0;

    /**
     * @brief Update the viewport
     *
     * Update the viewport of the object.
     *
     * @param _viewport New viewport
     */
    virtual void update_viewport(Viewport _viewport);

    /**
     * @brief Get the viewport
     *
     * Get the viewport of the object.
     *
     * @return Viewport
     */
    [[nodiscard]] Viewport get_viewport() const;

    /**
     * @brief On mouse button callback
     *
     * Callback for mouse button event.
     *
     * @param button Button
     * @param action Action
     * @param mods Mods
     * @param x X coordinate
     * @param y Y coordinate
     */
    virtual void on_mouse_button(
            [[maybe_unused]] int button,
            [[maybe_unused]] int action,
            [[maybe_unused]] int mods,
            [[maybe_unused]] double x,
            [[maybe_unused]] double y
    ) {}

    /**
     * @brief On cursor position callback
     *
     * Callback for cursor position event.
     *
     * @param x X coordinate
     * @param y Y coordinate
     */
    virtual void on_cursor_position(
            [[maybe_unused]] double x,
            [[maybe_unused]] double y
    ) {}

protected:
    /// Viewport
    Viewport viewport;
};


#endif //LSYSTEMS_DRAWABLE_HPP
