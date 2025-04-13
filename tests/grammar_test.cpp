#include <gtest/gtest.h>
#include "grammar.h"
#include "test_properties.hpp"

void ASSERT_PRODUCTIONS_EQ(
        const std::vector<Production> &productions,
        const std::vector<Production> &expected_productions
);

void ASSERT_PRODUCTIONS_DATA_EQ(
        const OpenGLReadyProductions &productions,
        const std::vector<Production> &expected_productions
);

TEST(Grammar, GrammarCreation) {
    auto grammar = load_grammar(FIXTURE_PATH("resources/all_basic_abc.ls"));

    ASSERT_NE(grammar, nullptr);

    ASSERT_EQ(grammar->get_property_float("step"), 10.0);
    ASSERT_EQ(grammar->get_property_size_t("depth"), 20);
    ASSERT_EQ(grammar->get_property_float("delta"), 90);

    auto productions = grammar->get_raw_productions();
    auto expected_productions = std::vector<Production>{
            {'B', "AA", {}, {}, {0, 0}},
            {'A', "BB", {}, {}, {0, 0}},
            {'C', "A",  {}, {}, {0}},
            {'C', "AA", {}, {}, {0, 0}}
    };

    ASSERT_PRODUCTIONS_EQ(productions, expected_productions);

    auto opengl_data = grammar->get_opengl_ready_productions();
    ASSERT_PRODUCTIONS_DATA_EQ(opengl_data, expected_productions);
}

TEST(Grammar, GrammarContextCheck) {
    auto grammar = load_grammar(FIXTURE_PATH("resources/basic_context.ls"));

    ASSERT_NE(grammar, nullptr);

    ASSERT_EQ(grammar->get_property_float("step"), 10.0);
    ASSERT_EQ(grammar->get_property_size_t("depth"), 20);
    ASSERT_EQ(grammar->get_property_float("delta"), 45);

    ASSERT_EQ(grammar->get_axiom()->axiom, "aAa");

    auto productions = grammar->get_raw_productions();
    auto expected_productions = std::vector<Production>{
            {'A', "AA", "a", "a", {0, 0}},
            {'A', "LA", "a", "",  {0, 0}},
            {'A', "AL", "",  "a", {0, 0}},
            {'L', "LL", "",  "",  {0, 0}},
            {'L', "FF", "",  "",  {0, 0}}
    };
    ASSERT_PRODUCTIONS_EQ(productions, expected_productions);

    auto opengl_data = grammar->get_opengl_ready_productions();
    ASSERT_PRODUCTIONS_DATA_EQ(opengl_data, expected_productions);
}

TEST(Grammar, GrammarLookBackCheck) {
    auto grammar = load_grammar(FIXTURE_PATH("resources/lookback_computation.ls"));

    ASSERT_NE(grammar, nullptr);

    ASSERT_EQ(grammar->get_property_float("step"), 100.0);
    ASSERT_EQ(grammar->get_property_size_t("depth"), 7);
    ASSERT_EQ(grammar->get_property_float("delta"), 22.5);

    ASSERT_EQ(grammar->get_axiom()->axiom, "FP");

    LookBackTable expected_axiom_lookback = {0, 0};
    ASSERT_EQ(grammar->get_axiom()->look_back, expected_axiom_lookback);

    auto productions = grammar->get_raw_productions();
    auto expected_productions = std::vector<Production>{
            {'P', "D[+P][-P][^P][&P]", "", "", {0, 3, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -3, 3, 0, 0, -3}},
            {'D', "DF",                "", "", {0, 0}},
            {'A', "{AfA}",             "", "", {4, 0, 0, 0, -4}},
    };
    ASSERT_PRODUCTIONS_EQ(productions, expected_productions);

    auto opengl_data = grammar->get_opengl_ready_productions();
    ASSERT_PRODUCTIONS_DATA_EQ(opengl_data, expected_productions);
}

/* UTILS */

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

std::vector<int32_t> calc_expected_predecessors(
        const std::vector<Production> &productions
) {
    std::vector<int32_t> predecessors;
    predecessors.reserve(productions.size());
    for (const auto &prod: productions) {
        predecessors.push_back(static_cast<int32_t>(prod.predecessor));
    }
    return predecessors;
}

std::vector<int32_t> calc_expected_lookback(
        const std::vector<Production> &productions
) {
    std::vector<int32_t> lookback_result;
    for (const auto &prod: productions) {
        auto &lookback = prod.look_back;
        lookback_result.insert(lookback_result.end(), lookback.begin(), lookback.end());
    }
    return lookback_result;
}

std::tuple<std::vector<int32_t>, std::vector<int32_t>, std::vector<int32_t>> calc_expected_triple(
        const std::vector<Production> &productions,
        const std::function<const std::string *(const Production &)> &base
) {
    std::vector<int32_t> sizes;
    sizes.reserve(productions.size());
    std::vector<int32_t> offsets;
    offsets.reserve(productions.size());
    std::vector<int32_t> data;
    for (const auto &prod: productions) {
        auto prop = base(prod);
        auto size = static_cast<int32_t>(prop->size());
        sizes.push_back(size);
        offsets.push_back(static_cast<int32_t>(data.size()));
        auto prop_casted = string_to_int32_t_vector(*prop);
        data.insert(data.end(), prop_casted.begin(), prop_casted.end());
    }
    return {sizes, offsets, data};
}

void ASSERT_PRODUCTIONS_DATA_EQ(
        const OpenGLReadyProductions &productions,
        const std::vector<Production> &expected_productions
) {
    auto expected_predecessors = calc_expected_predecessors(expected_productions);
    auto expected_lookback = calc_expected_lookback(expected_productions);
    auto [expected_successors_sizes, expected_successors_offsets, expected_successors_data] =
            calc_expected_triple(expected_productions, [](const Production &p) { return &p.successor; });
    auto [expected_left_context_sizes, expected_left_context_offsets, expected_left_context_data] =
            calc_expected_triple(expected_productions, [](const Production &p) { return &p.left_context; });
    auto [expected_right_context_sizes, expected_right_context_offsets, expected_right_context_data] =
            calc_expected_triple(expected_productions, [](const Production &p) { return &p.right_context; });

    ASSERT_EQ(*productions.predecessors, expected_predecessors);
    ASSERT_EQ(*productions.look_back, expected_lookback);
    ASSERT_EQ(*productions.successors_sizes, expected_successors_sizes);
    ASSERT_EQ(*productions.successors_offsets, expected_successors_offsets);
    ASSERT_EQ(*productions.successors_data, expected_successors_data);
    ASSERT_EQ(*productions.left_context_sizes, expected_left_context_sizes);
    ASSERT_EQ(*productions.left_context_offsets, expected_left_context_offsets);
    ASSERT_EQ(*productions.left_context_data, expected_left_context_data);
    ASSERT_EQ(*productions.right_context_sizes, expected_right_context_sizes);
    ASSERT_EQ(*productions.right_context_offsets, expected_right_context_offsets);
    ASSERT_EQ(*productions.right_context_data, expected_right_context_data);
}

