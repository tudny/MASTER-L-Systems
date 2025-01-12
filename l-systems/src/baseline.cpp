#include "baseline.h"

#include <cmath>
#include <iostream>

void static multiply_node_inplace(MatrixTree::Node<glm::mat4> &node) {
    if (node.parent != nullptr) {
        node.data = node.data * node.parent->data;
    }

    for (auto &child : node.children) {
        multiply_node_inplace(*child);
    }
}

void MatrixTree::multiply_tree_inplace(MatrixTree::Tree<glm::mat4> &tree) {
    if (tree.root == nullptr) {
        return;
    }

    multiply_node_inplace(*tree.root);
}

void collect_node_data_inplace(MatrixTree::Node<glm::mat4> &node, std::vector<glm::mat4> &instances) {
    instances.push_back(node.data);

    for (auto &child : node.children) {
        collect_node_data_inplace(*child, instances);
    }
}

void MatrixTree::collect_data_inplace(MatrixTree::Tree<glm::mat4> &tree, std::vector<glm::mat4> &instances) {
    if (tree.root == nullptr) {
        return;
    }

    collect_node_data_inplace(*tree.root, instances);
}

glm::mat4 Turtle::rotate_over_heading(float angle) {
    return glm::mat4{
        1., 0., 0., 0.,
        0., std::cos(angle), std::sin(angle), 0.,
        0., -std::sin(angle), std::cos(angle), 0.,
        0., 0., 0., 1.
    };
}

glm::mat4 Turtle::rotate_over_left(float angle) {
    return glm::mat4{
        std::cos(angle), 0., std::sin(angle), 0.,
        0., 1., 0., 0.,
        -std::sin(angle), 0., std::cos(angle), 0.,
        0., 0., 0., 1.
    };
}

glm::mat4 Turtle::rotate_over_up(float angle) {
    return glm::mat4{
        std::cos(angle), -std::sin(angle), 0., 0.,
        std::sin(angle), std::cos(angle), 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1.
    };
}

glm::mat4 Turtle::move_forward(float distance) {
    return glm::mat4{
        1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        distance, 0., 0., 1.
    };
}

void print_mat4_row(const glm::mat4 &m) {
    std::cout << "Matrix:" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << m[i][0] << " " << m[i][1] << " " << m[i][2] << " " << m[i][3] << std::endl;
    }
    std::cout << std::endl;
}

std::vector<glm::mat4> TempSpace::sample_instances() {
    auto init_state = Turtle::State{
            glm::vec3(1, 0, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(0, 0, 1),
            glm::vec3(0, 0, -10)
    };

    auto d = 5.0f;

    auto root = MatrixTree::Node<glm::mat4>{init_state.state};

    auto root_up = MatrixTree::Node<glm::mat4>{Turtle::move_forward(d)};
    root_up.parent = &root;
    root.children.push_back(&root_up);

    auto root_up_left = MatrixTree::Node<glm::mat4>{Turtle::move_forward(d) * Turtle::rotate_over_left(glm::radians(45.0f))};
    root_up_left.parent = &root_up;
    root_up.children.push_back(&root_up_left);

    auto root_up_right = MatrixTree::Node<glm::mat4>{Turtle::move_forward(d) * Turtle::rotate_over_left(glm::radians(-45.0f))};
    root_up_right.parent = &root_up;
    root_up.children.push_back(&root_up_right);

    auto root_up_left_up = MatrixTree::Node<glm::mat4>{Turtle::move_forward(d) * Turtle::rotate_over_left(glm::radians(45.0f))};
    root_up_left_up.parent = &root_up_right;
    root_up_right.children.push_back(&root_up_left_up);

//    auto root_up_left_up_left = MatrixTree::Node<glm::mat4>{Turtle::rotate_over_left(glm::radians(45.0f)) * Turtle::move_forward(d)};
//    root_up_left_up_left.parent = &root_up_left_up;
//    root_up_left_up.children.push_back(&root_up_left_up_left);

//    auto root_up_left_up_right = MatrixTree::Node<glm::mat4>{Turtle::rotate_over_left(glm::radians(-45.0f)) * Turtle::move_forward(d)};
//    root_up_left_up_right.parent = &root_up_left_up;
//    root_up_left_up.children.push_back(&root_up_left_up_right);

    auto tree = MatrixTree::Tree<glm::mat4>{&root};
    multiply_tree_inplace(tree);

    std::vector<glm::mat4> instances;
    collect_data_inplace(tree, instances);

    std::cout << "instances.size() = " << instances.size() << std::endl;

    for (auto &instance : instances) {
        print_mat4_row(instance);
    }

    return instances;
}
