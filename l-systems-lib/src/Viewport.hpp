/** @file
 * @brief Viewport class declaration
 *
 * This file contains the declaration of the Viewport class.
 *
 * @author Aleksander Tudruj <at429630@students.mimuw.edu.pl>
 * @date 07.07.2024
*/

#ifndef LSYSTEMS_VIEWPORT_HPP
#define LSYSTEMS_VIEWPORT_HPP


#include <glm/glm.hpp>

/// Viewport class abstracts the viewport
class Viewport {
public:

    /// Left position
    int left;
    /// Top position
    int top;
    /// Width
    int width;
    /// Height
    int height;
    /// Window width
    int window_width;
    /// Window height
    int window_height;

    /**
     * @brief Window to local fixed ratio
     *
     * TODO: Add description
     *
     * @param x
     * @param y
     * @return glm::vec2
     */
    [[nodiscard]] glm::vec2 window_to_local_fixed_ratio(double x, double y) const;

    /**
     * @brief Window to local stretch
     *
     * TODO: Add description
     *
     * @param x
     * @param y
     * @return glm::vec2
     */
    [[nodiscard]] glm::vec2 window_to_local_stretch(double x, double y) const;


    /**
     * @brief Local fixed ratio to standard square
     *
     * TODO: Add description
     *
     * @param x
     * @param y
     * @return glm::vec2
     */
    [[nodiscard]] glm::mat3 local_fixed_ratio_to_standard_square() const;

    /**
     * @brief Local stretch to standard square
     *
     * TODO: Add description
     *
     * @param x
     * @param y
     * @return glm::vec2
     */
    [[nodiscard]] glm::mat3 local_stretch_to_standard_square() const;
};


#endif //LSYSTEMS_VIEWPORT_HPP
