#ifndef LSYSTEMS_BASELINE_H
#define LSYSTEMS_BASELINE_H

#include <vector>
#include <stdexcept>
#include <iostream>
#include "glm/glm.hpp"

namespace MatrixTree {
    class TransformationMatrix {
        static size_t constexpr SIZE = 16;
    public:
        float data[SIZE]{0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};

    public:
        explicit TransformationMatrix(float _data) {
            for (float &i: this->data) {
                i = _data;
            }
        }

        TransformationMatrix(std::initializer_list<float> list) {
            if (list.size() == 0 || list.size() == 1) {
                float _data = (list.size() == 0) ? 0 : *list.begin();
                for (float &i: this->data) {
                    i = _data;
                }
                return;
            }

            if (list.size() != SIZE) {
                std::cout << "list.size() = " << list.size() << std::endl;
                throw std::runtime_error("Invalid size for transformation matrix");
            }

            size_t i = 0;
            for (auto &value: list) {
                data[i++] = value;
            }
        }

        TransformationMatrix operator*(const TransformationMatrix &other) const {
            return {
                    data[0] * other.data[0] + data[1] * other.data[4] + data[2] * other.data[8] + data[3] * other.data[12],
                    data[0] * other.data[1] + data[1] * other.data[5] + data[2] * other.data[9] + data[3] * other.data[13],
                    data[0] * other.data[2] + data[1] * other.data[6] + data[2] * other.data[10] + data[3] * other.data[14],
                    data[0] * other.data[3] + data[1] * other.data[7] + data[2] * other.data[11] + data[3] * other.data[15],
                    data[4] * other.data[0] + data[5] * other.data[4] + data[6] * other.data[8] + data[7] * other.data[12],
                    data[4] * other.data[1] + data[5] * other.data[5] + data[6] * other.data[9] + data[7] * other.data[13],
                    data[4] * other.data[2] + data[5] * other.data[6] + data[6] * other.data[10] + data[7] * other.data[14],
                    data[4] * other.data[3] + data[5] * other.data[7] + data[6] * other.data[11] + data[7] * other.data[15],
                    data[8] * other.data[0] + data[9] * other.data[4] + data[10] * other.data[8] + data[11] * other.data[12],
                    data[8] * other.data[1] + data[9] * other.data[5] + data[10] * other.data[9] + data[11] * other.data[13],
                    data[8] * other.data[2] + data[9] * other.data[6] + data[10] * other.data[10] + data[11] * other.data[14],
                    data[8] * other.data[3] + data[9] * other.data[7] + data[10] * other.data[11] + data[11] * other.data[15],
                    data[12] * other.data[0] + data[13] * other.data[4] + data[14] * other.data[8] + data[15] * other.data[12],
                    data[12] * other.data[1] + data[13] * other.data[5] + data[14] * other.data[9] + data[15] * other.data[13],
                    data[12] * other.data[2] + data[13] * other.data[6] + data[14] * other.data[10] + data[15] * other.data[14],
                    data[12] * other.data[3] + data[13] * other.data[7] + data[14] * other.data[11] + data[15] * other.data[15]
            };
        }

        void print() const {
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    std::cout << data[i * 4 + j] << " ";
                }
                std::cout << std::endl;
            }
        }

        [[nodiscard]] glm::mat4 to_glm_mat4() const {
            // MatrixTree::TransformationMatrix is row-major, glm::mat4 is column-major
            return glm::mat4{
                    data[0], data[1], data[2], data[3],
                    data[4], data[5], data[6], data[7],
                    data[8], data[9], data[10], data[11],
                    data[12], data[13], data[14], data[15]
            };
        }
    };

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

    void multiply_tree_inplace(Tree<TransformationMatrix> &tree);

    void collect_data_inplace(Tree<TransformationMatrix> &tree, std::vector<TransformationMatrix> &instances);
};

namespace Turtle {

    using Operator = MatrixTree::TransformationMatrix;

    class State {
    public:
        // First three columns are named H, L, U - heading, left, up
        // Last (4th) column is the position in 3d space
        MatrixTree::TransformationMatrix state{1.};

        State() {
            state = MatrixTree::TransformationMatrix{1};
            state.data[15] = 1;
        }

        State(const glm::vec3 &h, const glm::vec3 &l, const glm::vec3 &u, const glm::vec3 &p) {
            validate(h, l, u);
            state = MatrixTree::TransformationMatrix{
                    h.x, h.y, h.z, 0,
                    l.x, l.y, l.z, 0,
                    u.x, u.y, u.z, 0,
                    p.x, p.y, p.z, 1
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
