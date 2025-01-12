#ifndef LSYSTEMS_BASELINE_H
#define LSYSTEMS_BASELINE_H

#include <vector>
#include <stdexcept>
#include "glm/glm.hpp"

namespace MatrixTree {
    template<typename T>
    class Node {
    public:
        T data;
        Node *parent{};
        std::vector<Node *> children{};

        Node(T data) : data(std::move(data)) {}
    };

    template<typename T>
    class Tree {
    public:
        Node<T> *root;
    };

    void multiply_tree_inplace(Tree<glm::mat4> &tree);

    void collect_data_inplace(Tree<glm::mat4> &tree, std::vector<glm::mat4> &instances);
};

namespace Turtle {

    using Operator = glm::mat4;

    class State {
    public:
        // First three columns are named H, L, U - heading, left, up
        // Last (4th) column is the position in 3d space
        glm::mat4 state{};

        State() {
            state = glm::mat4(1.0f);
            state[3][3] = 0.0f;
        }

        State(const glm::mat4 &state) : state(state) {}

        State(const glm::vec3 &h, const glm::vec3 &l, const glm::vec3 &u, const glm::vec3 &p) {
            validate(h, l, u);
            state = glm::mat4{
                    h.x, h.y, h.z, 0.0f,
                    l.x, l.y, l.z, 0.0f,
                    u.x, u.y, u.z, 0.0f,
                    p.x, p.y, p.z, 1.0f
            };
        }

        static void validate(const glm::vec3 &h, const glm::vec3 &l, const glm::vec3 &u) {
            if (glm::dot(h, l) != 0) {
                throw std::runtime_error("Heading and left vectors must be orthogonal");
            }

            if (glm::dot(h, u) != 0) {
                throw std::runtime_error("Heading and up vectors must be orthogonal");
            }

            if (glm::dot(l, u) != 0) {
                throw std::runtime_error("Left and up vectors must be orthogonal");
            }

            if (glm::dot(h, h) != 1) {
                throw std::runtime_error("Heading vector must be normalized");
            }

            if (glm::dot(l, l) != 1) {
                throw std::runtime_error("Left vector must be normalized");
            }

            if (glm::dot(u, u) != 1) {
                throw std::runtime_error("Up vector must be normalized");
            }

            if (glm::cross(h, l) != u) {
                throw std::runtime_error("Heading, left and up vectors must form a right-handed coordinate system");
            }
        }

        void apply(const Operator &op) {
            state = state * op;
        }

        State copy() const {
            return State{state};
        }
    };

    Operator rotate_over_heading(float angle);

    Operator rotate_over_left(float angle);

    Operator rotate_over_up(float angle);

    Operator move_forward(float distance);
};

namespace TempSpace {
    std::vector<glm::mat4> sample_instances();
}

#endif //LSYSTEMS_BASELINE_H
