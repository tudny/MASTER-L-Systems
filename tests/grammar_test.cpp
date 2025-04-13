#include <gtest/gtest.h>
#include "grammar.h"

void ASSERT_PRODUCTIONS_EQ(
        const std::vector<Production> &productions,
        const std::vector<Production> &expected_productions
) {
    ASSERT_EQ(productions.size(), expected_productions.size());
    for (size_t i = 0; i < productions.size(); i++) {
        ASSERT_EQ(productions[i].predecessor, expected_productions[i].predecessor);
        ASSERT_EQ(productions[i].successor, expected_productions[i].successor);
        ASSERT_EQ(productions[i].left_context, expected_productions[i].left_context);
        ASSERT_EQ(productions[i].right_context, expected_productions[i].right_context);
        ASSERT_EQ(productions[i].look_back, expected_productions[i].look_back);
    }
}

std::vector<int32_t> string_to_int32_t_vector(const std::string &str) {
    std::vector<int32_t> result;
    for (char c: str) {
        result.push_back(static_cast<int32_t>(c));
    }
    return result;
}

TEST(Grammar, GrammarCreation) {
    auto grammar = load_grammar("resources/all_basic_abc.ls");

    ASSERT_NE(grammar, nullptr);

    ASSERT_EQ(grammar->get_property_float("step"), 10.0);
    ASSERT_EQ(grammar->get_property_size_t("depth"), 20);
    ASSERT_EQ(grammar->get_property_float("delta"), 90);

    auto productions = grammar->get_raw_productions();
    auto expected_productions = std::vector<Production>{
            {'A', "BB", {}, {}, {0, 0}},
            {'B', "AA", {}, {}, {0, 0}},
            {'C', "A",  {}, {}, {0}},
            {'C', "AA", {}, {}, {0, 0}}
    };

    ASSERT_PRODUCTIONS_EQ(productions, expected_productions);

    auto opengl_data = grammar->get_opengl_ready_productions();

    std::vector<int32_t> expected_predecessors = {'A', 'B', 'C', 'C'};
    ASSERT_EQ(*opengl_data.predecessors, expected_predecessors);

    std::vector<int32_t> expected_look_back = {0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(*opengl_data.look_back, expected_look_back);

    std::vector<int32_t> expected_successors_sizes = {2, 2, 1, 2};
    ASSERT_EQ(*opengl_data.successors_sizes, expected_successors_sizes);

    std::vector<int32_t> expected_successors_offsets = {0, 2, 4, 5};
    ASSERT_EQ(*opengl_data.successors_offsets, expected_successors_offsets);

    std::vector<int32_t> expected_successors_data = {'B', 'B', 'A', 'A', 'A', 'A', 'A'};
    ASSERT_EQ(*opengl_data.successors_data, expected_successors_data);

    std::vector<int32_t> expected_left_context_sizes = {0, 0, 0, 0};
    ASSERT_EQ(*opengl_data.left_context_sizes, expected_left_context_sizes);

    std::vector<int32_t> expected_left_context_offsets = {0, 0, 0, 0};
    ASSERT_EQ(*opengl_data.left_context_offsets, expected_left_context_offsets);

    std::vector<int32_t> expected_left_context_data = {};
    ASSERT_EQ(*opengl_data.left_context_data, expected_left_context_data);

    std::vector<int32_t> expected_right_context_sizes = {0, 0, 0, 0};
    ASSERT_EQ(*opengl_data.right_context_sizes, expected_right_context_sizes);

    std::vector<int32_t> expected_right_context_offsets = {0, 0, 0, 0};
    ASSERT_EQ(*opengl_data.right_context_offsets, expected_right_context_offsets);

    std::vector<int32_t> expected_right_context_data = {};
    ASSERT_EQ(*opengl_data.right_context_data, expected_right_context_data);
}

TEST(Grammar, GrammarContextCheck) {
    auto grammar = load_grammar("resources/basic_context.ls");

    ASSERT_NE(grammar, nullptr);

    ASSERT_EQ(grammar->get_property_float("step"), 10.0);
    ASSERT_EQ(grammar->get_property_size_t("depth"), 20);
    ASSERT_EQ(grammar->get_property_float("delta"), 45);

    ASSERT_EQ(grammar->get_axiom()->axiom, "aAa");

    auto productions = grammar->get_raw_productions();
    auto expected_productions = std::vector<Production>{
            {'A', "AA", "a", "a", {0, 0}},
            {'A', "AL", "",  "a", {0, 0}},
            {'A', "LA", "a", "",  {0, 0}},
            {'L', "FF", "",  "",  {0, 0}},
            {'L', "LL", "",  "",  {0, 0}}
    };
    ASSERT_PRODUCTIONS_EQ(productions, expected_productions);

    auto opengl_data = grammar->get_opengl_ready_productions();

    std::vector<int32_t> expected_predecessors = {'A', 'A', 'A', 'L', 'L'};
    ASSERT_EQ(*opengl_data.predecessors, expected_predecessors);

    std::vector<int32_t> expected_look_back = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(*opengl_data.look_back, expected_look_back);

    std::vector<int32_t> expected_successors_sizes = {2, 2, 2, 2, 2};
    ASSERT_EQ(*opengl_data.successors_sizes, expected_successors_sizes);

    std::vector<int32_t> expected_successors_offsets = {0, 2, 4, 6, 8};
    ASSERT_EQ(*opengl_data.successors_offsets, expected_successors_offsets);

    std::vector<int32_t> expected_successors_data = {'A', 'A', 'L', 'A', 'L', 'F', 'F', 'L', 'L'};
    ASSERT_EQ(*opengl_data.successors_data, expected_successors_data);

    std::vector<int32_t> expected_left_context_sizes = {1, 0, 1, 0, 0};
    ASSERT_EQ(*opengl_data.left_context_sizes, expected_left_context_sizes);

    std::vector<int32_t> expected_left_context_offsets = {0, 1, 1, 2, 2};
    ASSERT_EQ(*opengl_data.left_context_offsets, expected_left_context_offsets);

    std::vector<int32_t> expected_left_context_data = {'a', 'a'};
    ASSERT_EQ(*opengl_data.left_context_data, expected_left_context_data);

    std::vector<int32_t> expected_right_context_sizes = {1, 1, 0, 0, 0};
    ASSERT_EQ(*opengl_data.right_context_sizes, expected_right_context_sizes);

    std::vector<int32_t> expected_right_context_offsets = {0, 1, 2, 2, 2};
    ASSERT_EQ(*opengl_data.right_context_offsets, expected_right_context_offsets);

    std::vector<int32_t> expected_right_context_data = {'a', 'a'};
    ASSERT_EQ(*opengl_data.right_context_data, expected_right_context_data);
}

TEST(Grammar, GrammarLookBackCheck) {
    auto grammar = load_grammar("resources/lookback_computation.ls");

    ASSERT_NE(grammar, nullptr);

    ASSERT_EQ(grammar->get_property_float("step"), 100.0);
    ASSERT_EQ(grammar->get_property_size_t("depth"), 7);
    ASSERT_EQ(grammar->get_property_float("delta"), 22.5);

    ASSERT_EQ(grammar->get_axiom()->axiom, "FP");

    LookBackTable expected_axiom_lookback = {0, 0};
    ASSERT_EQ(grammar->get_axiom()->look_back, expected_axiom_lookback);

    auto productions = grammar->get_raw_productions();
    auto expected_productions = std::vector<Production>{
            {'A', "{AfA}",             "", "", {4, 0, 0, 0, -4}},
            {'D', "DF",                "", "", {0, 0}},
            {'P', "D[+P][-P][^P][&P]", "", "", {0, 3, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -3}},
    };
    ASSERT_PRODUCTIONS_EQ(productions, expected_productions);

    auto opengl_data = grammar->get_opengl_ready_productions();

    std::vector<int32_t> expected_predecessors = {'A', 'D', 'P'};
    ASSERT_EQ(*opengl_data.predecessors, expected_predecessors);

    std::vector<int32_t> expected_look_back = {
            4, 0, 0, 0, -4,
            0, 0,
            0, 3, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -3
    };
    ASSERT_EQ(*opengl_data.look_back, expected_look_back);

    std::vector<int32_t> expected_successors_sizes = {5, 2, 17};
    ASSERT_EQ(*opengl_data.successors_sizes, expected_successors_sizes);

    std::vector<int32_t> expected_successors_offsets = {0, 5, 7};
    ASSERT_EQ(*opengl_data.successors_offsets, expected_successors_offsets);

    std::vector<int32_t> expected_successors_data = string_to_int32_t_vector("{AfA}DFD[+P][-P][^P][&P]");
    ASSERT_EQ(*opengl_data.successors_data, expected_successors_data);

    std::vector<int32_t> expected_left_context_sizes = {0, 0, 0};
    ASSERT_EQ(*opengl_data.left_context_sizes, expected_left_context_sizes);

    std::vector<int32_t> expected_left_context_offsets = {0, 0, 0};
    ASSERT_EQ(*opengl_data.left_context_offsets, expected_left_context_offsets);

    std::vector<int32_t> expected_left_context_data = {};
    ASSERT_EQ(*opengl_data.left_context_data, expected_left_context_data);

    std::vector<int32_t> expected_right_context_sizes = {0, 0, 0};
    ASSERT_EQ(*opengl_data.right_context_sizes, expected_right_context_sizes);

    std::vector<int32_t> expected_right_context_offsets = {0, 0, 0};
    ASSERT_EQ(*opengl_data.right_context_offsets, expected_right_context_offsets);

    std::vector<int32_t> expected_right_context_data = {};
    ASSERT_EQ(*opengl_data.right_context_data, expected_right_context_data);
}
