#include "baseline.hpp"
#include "grammar.h"
#include "glm/ext/matrix_transform.hpp"
#include "debug.hpp"

#include <cmath>
#include <iostream>
#include <stack>

void static multiply_node_inplace(MatrixTree::Node<MatrixTree::TransformationMatrix> &node) {
    if (node.parent != nullptr) {
        node.data = node.data * node.parent->data;
    }

    for (auto &child: node.children) {
        multiply_node_inplace(*child);
    }
}

void MatrixTree::multiply_tree_inplace(MatrixTree::Tree<MatrixTree::TransformationMatrix> &tree) {
    if (tree.root == nullptr) {
        return;
    }

    multiply_node_inplace(*tree.root);
}

void collect_node_data_inplace(MatrixTree::Node<MatrixTree::TransformationMatrix> &node,
                               std::vector<MatrixTree::TransformationMatrix> &instances) {
    instances.push_back(node.data);

    for (auto &child: node.children) {
        collect_node_data_inplace(*child, instances);
    }
}

void MatrixTree::collect_data_inplace(MatrixTree::Tree<MatrixTree::TransformationMatrix> &tree,
                                      std::vector<MatrixTree::TransformationMatrix> &instances) {
    if (tree.root == nullptr) {
        return;
    }

    collect_node_data_inplace(*tree.root, instances);
}

MatrixTree::TransformationMatrix Turtle::rotate_over_heading(float angle) {
    return MatrixTree::TransformationMatrix{
            1., 0., 0., 0.,
            0., std::cos(angle), std::sin(angle), 0.,
            0., -std::sin(angle), std::cos(angle), 0.,
            0., 0., 0., 1.
    };
}

MatrixTree::TransformationMatrix Turtle::rotate_over_left(float angle) {
    return MatrixTree::TransformationMatrix{
            std::cos(angle), 0., std::sin(angle), 0.,
            0., 1., 0., 0.,
            -std::sin(angle), 0., std::cos(angle), 0.,
            0., 0., 0., 1.
    };
}

MatrixTree::TransformationMatrix Turtle::rotate_over_up(float angle) {
    return MatrixTree::TransformationMatrix{
            std::cos(angle), -std::sin(angle), 0., 0.,
            std::sin(angle), std::cos(angle), 0., 0.,
            0., 0., 1., 0.,
            0., 0., 0., 1.
    };
}

MatrixTree::TransformationMatrix Turtle::move_forward(float distance) {
    return MatrixTree::TransformationMatrix{
            1., 0., 0., 0.,
            0., 1., 0., 0.,
            0., 0., 1., 0.,
            distance, 0., 0., 1.
    };
}

void print_mat4_row(const MatrixTree::TransformationMatrix &m) {
    std::cout << "Matrix:" << std::endl;
    m.print();
    std::cout << std::endl;
}

void print_node_matrix(MatrixTree::Node<MatrixTree::TransformationMatrix> &node, const std::string &node_name) {
    std::cout << "Node: " << node_name << std::endl;
    print_mat4_row(node.data);
}

std::vector<glm::mat4> TempSpace::sample_instances() {
    auto init_state = Turtle::State{
            glm::vec3(0, 1, 0),
            glm::vec3(-1, 0, 0),
            glm::vec3(0, 0, 1),
            glm::vec3(0, -10, 0)
    };

    auto d = 5.0f;

    auto root = MatrixTree::Node<MatrixTree::TransformationMatrix>{init_state.state};
//    print_node_matrix(root, "root");

    auto root_up = MatrixTree::Node<MatrixTree::TransformationMatrix>{Turtle::move_forward(d)};
//    print_node_matrix(root_up, "root_up");
    root_up.parent = &root;
    root.children.push_back(&root_up);

    auto root_up_left = MatrixTree::Node<MatrixTree::TransformationMatrix>{Turtle::move_forward(d) * Turtle::rotate_over_up(glm::radians(-45.0f))};
    root_up_left.parent = &root_up;
    root_up.children.push_back(&root_up_left);

    auto root_up_right = MatrixTree::Node<MatrixTree::TransformationMatrix>{Turtle::move_forward(d) * Turtle::rotate_over_up(glm::radians(45.0f))};
    root_up_right.parent = &root_up;
    root_up.children.push_back(&root_up_right);

    auto root_up_right_up = MatrixTree::Node<MatrixTree::TransformationMatrix>{Turtle::move_forward(d) * Turtle::rotate_over_up(glm::radians(-45.0f))};
    root_up_right_up.parent = &root_up_right;
    root_up_right.children.push_back(&root_up_right_up);

    auto root_up_left_up_left = MatrixTree::Node<MatrixTree::TransformationMatrix>{Turtle::move_forward(d) * Turtle::rotate_over_up(glm::radians(-45.0f))};
    root_up_left_up_left.parent = &root_up_right_up;
    root_up_right_up.children.push_back(&root_up_left_up_left);

    auto root_up_left_up_right = MatrixTree::Node<MatrixTree::TransformationMatrix>{Turtle::move_forward(d) * Turtle::rotate_over_up(glm::radians(45.0f))};
    root_up_left_up_right.parent = &root_up_right_up;
    root_up_right_up.children.push_back(&root_up_left_up_right);

    auto tree = MatrixTree::Tree<MatrixTree::TransformationMatrix>{&root};
    multiply_tree_inplace(tree);

    std::vector<MatrixTree::TransformationMatrix> instances;
    collect_data_inplace(tree, instances);

    std::cout << "instances.size() = " << instances.size() << std::endl;

    std::vector<glm::mat4> instances_as_mat4;
    instances_as_mat4.reserve(instances.size());
    for (auto &instance: instances) {
        instances_as_mat4.push_back(instance.to_glm_mat4());
    }

    return instances_as_mat4;
}

static inline bool is_instance(char c, bool is_filling) {
    return c == 'F' or (is_filling and c == 'f');
}

static size_t count_instances(const std::string &instances_str) {
    size_t instances = 0;
    bool is_filling = false;
    for (auto c: instances_str) {
        if (c == '{') {
            is_filling = true;
        } else if (c == '}') {
            is_filling = false;
        }
        if (is_instance(c, is_filling)) {
            instances++;
        }
    }
    return instances;
}

std::vector<glm::mat4> TempSpace::grammar_instances(const GrammarPtr& grammar) {
    auto production_str = grammar->cpu_produce();
    auto instances_count = count_instances(production_str);

    float delta = glm::radians(grammar->get_property_float("delta"));
    float step = grammar->get_property_float("step");

    float downset = grammar->get_property_float("downset");

    auto init = Turtle::State{
            glm::vec3(0, 1, 0),
            glm::vec3(-1, 0, 0),
            glm::vec3(0, 0, 1),
            glm::vec3(0, -downset, 0)
    };

    auto movement_mappings = std::unordered_map<char, MatrixTree::TransformationMatrix>{
            {'F', Turtle::move_forward(step)},
            {'f', Turtle::move_forward(step)},
            {'+', Turtle::rotate_over_up(delta)},
            {'-', Turtle::rotate_over_up(-delta)},
            {'&', Turtle::rotate_over_left(delta)},
            {'^', Turtle::rotate_over_left(-delta)},
            {'/', Turtle::rotate_over_heading(delta)},
            {'\\', Turtle::rotate_over_heading(-delta)},
            {'|', Turtle::rotate_over_up(glm::radians(180.0f))}
    };

    MatrixTree::TransformationMatrix last_operation = init.state;
    std::vector<glm::mat4> instances;
    instances.reserve(instances_count);

    auto move_down = glm::translate(glm::mat4(1.0), glm::vec3(-0.5f, 0.0, 0.0f));
    auto scale_y_by_step = glm::scale(glm::mat4(1.0), glm::vec3(-step, 1.0f, 1.0f));
    auto move_back_up = glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.0f, 0.0f));
    auto translation = move_down * scale_y_by_step * move_back_up;

    std::stack<MatrixTree::TransformationMatrix> instances_stack;
    bool is_filling = false;

    for (char move : production_str) {
        if (move == '[') {
            instances_stack.push(last_operation);
            continue;
        } else if (move == ']') {
            last_operation = instances_stack.top();
            instances_stack.pop();
            continue;
        }

        if (move == '{') {
            is_filling = true;
            continue;
        } else if (move == '}') {
            is_filling = false;
            continue;
        }

        auto it = movement_mappings.find(move);
        if (it != movement_mappings.end()) {
            last_operation = it->second * last_operation;
            if (is_instance(move, is_filling)) {
                instances.push_back(last_operation.to_glm_mat4() * translation);
            }
        }
    }

    return instances;
}
