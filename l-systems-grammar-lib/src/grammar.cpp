#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <utility>
#include <stack>
#include "grammar.h"
#include "str_utils.h"
#include "memory_utils.h"
#include "Parser.H"
#include "Absyn.H"
#include "Skeleton.H"

const char STACK_OPENING_BRACKET = '[';
const char STACK_CLOSING_BRACKET = ']';
const char LEAF_OPENING_BRACKET = '{';
const char LEAF_CLOSING_BRACKET = '}';

constexpr const char *REQUIRED_PROPS[] = {"delta", "step", "depth"};

constexpr const char *PROPERTY_SEPARATOR = ":=";
constexpr const char *PRODUCTION_SEPARATOR = "->";

/*
 * ASCII codes for
 * [ = 01011011
 * ] = 01011101
 * { = 01111011
 * } = 01111101
 *
 * Maps [ -> ] and ] -> [ and { -> } and } -> { by bit manipulation
 * */
constexpr char matching_bracket(const char bracket) {
    return (char) (((int) bracket) ^ 0b0110);
}

constexpr bool is_opening_bracket(const char bracket) {
    return bracket == STACK_OPENING_BRACKET || bracket == LEAF_OPENING_BRACKET;
}

constexpr bool is_closing_bracket(const char bracket) {
    return bracket == STACK_CLOSING_BRACKET || bracket == LEAF_CLOSING_BRACKET;
}

/*
 * We define look_back table as the look-up for the tree nodes in the generated L-system word
 * For now we limit the size of the produced words to 2^31 - 1
 * Negative look_back indicates that the opening bracket is n steps before us
 * Positive look_back indicates that the closing bracket is n steps ahead of us
 * */
static std::vector<int32_t> compute_look_back(const std::string &successor) {
    std::vector<int32_t> look_back(successor.size(), 0);
    std::stack<std::pair<int32_t, char>> stack;

    for (int32_t i = 0; i < static_cast<int32_t>(successor.size()); i++) {
        if (is_opening_bracket(successor[i])) {
            stack.emplace(i, matching_bracket(successor[i]));
        } else if (is_closing_bracket(successor[i])) {
            if (stack.empty()) {
                throw std::runtime_error("Unbalanced brackets in successor: " + successor);
            }
            auto [bracket_index, expected_bracket] = stack.top();
            if (successor[i] != expected_bracket) {
                throw std::runtime_error("Unbalanced brackets in successor: " + successor);
            }
            stack.pop();
            look_back[i] = bracket_index - i;
            look_back[bracket_index] = i - bracket_index;
        }
    }

    if (!stack.empty()) {
        throw std::runtime_error("Unbalanced brackets in successor: " + successor);
    }

    return look_back;
}

class GrammarBuilder {
public:
    GrammarBuilder() = default;

    void add_property(const std::string &name, float value) {
        properties.push_back({name, value});
    }

    void set_axiom(const std::string &_axiom, const LookBackTable &look_back) {
        this->axiom = {_axiom, look_back};
    }

    void add_production(char predecessor,
                        const std::string &successor,
                        const std::string &left_context,
                        const std::string &right_context,
                        LookBackTable look_back) {
        productions.push_back({predecessor, successor, left_context, right_context, std::move(look_back)});
    }

    GrammarPtr build() {
        for (const auto &prop: REQUIRED_PROPS) {
            if (std::find_if(properties.begin(), properties.end(), [&prop](const Property &p) {
                return p.name == prop;
            }) == properties.end()) {
                throw std::runtime_error("Missing required property: " + std::string(prop));
            }
        }

        return std::make_shared<Grammar>(properties, axiom, productions);
    }

private:
    std::vector<Property> properties;
    Axiom axiom;
    std::vector<Production> productions;
};

class GrammarVisitor : public Skeleton {
public:
    void visitASTLIProperty(ASTLIProperty *p) override {
        builder.add_property(
                p->prod_,
                static_cast<float>(p->integer_)
        );
    }

    void visitASTLDProperty(ASTLDProperty *p) override {
        builder.add_property(
                p->prod_,
                static_cast<float>(p->double_)
        );
    }

    void visitASTLAxiom(ASTLAxiom *p) override {
        builder.set_axiom(
                p->prod_,
                compute_look_back(p->prod_)
        );
    }

    void visitASTLRule(ASTLRule *p) override {
        append_production(
                p->prod_1,
                p->prod_2,
                std::string(),
                std::string()
        );
    }

    void visitASTLLeftRule(ASTLLeftRule *p) override {
        append_production(
                p->prod_2,
                p->prod_3,
                p->prod_1,
                std::string()
        );
    }

    void visitASTLRightRule(ASTLRightRule *p) override {
        append_production(
                p->prod_1,
                p->prod_3,
                std::string(),
                p->prod_2
        );
    }

    void visitASTLBothRule(ASTLBothRule *p) override {
        append_production(
                p->prod_2,
                p->prod_4,
                p->prod_1,
                p->prod_3
        );
    }

    void append_production(
            const std::string &predecessor,
            const std::string &successor,
            const std::string &left_context,
            const std::string &right_context
    ) {
        builder.add_production(
                of_string(predecessor),
                successor,
                left_context,
                right_context,
                compute_look_back(successor)
        );
    }

    static char of_string(const std::string &str) {
        if (str.size() != 1) {
            throw std::runtime_error("Expected single character, got: " + str);
        }
        return str[0];
    }

public:
    GrammarBuilder builder;
};

static GrammarPtr parse_grammar(ASTProgram *program) {
    std::shared_ptr<GrammarVisitor> visitor = std::make_shared<GrammarVisitor>();
    program->accept(visitor.get());
    auto grammar = visitor->builder.build();
    return grammar;
}

GrammarPtr load_grammar(const std::string &path) {
    FILE *input = fopen(path.c_str(), "r");
    if (!input) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    auto *parse_tree = pASTProgram(input);
    fclose(input);
    if (!parse_tree) {
        throw std::runtime_error("Failed to parse file: " + path);
    }
    return parse_grammar(parse_tree);
}

void Grammar::print(std::ostream &os) {
    auto sep = [](size_t n) { return std::string(n, ' '); };
    auto print_lookback = [&](auto &look_back) {
        os << sep(3) << "look_back := [";
        for (size_t i = 0; i < look_back.size(); i++) {
            os << look_back[i];
            if (i != look_back.size() - 1) {
                os << ", ";
            }
        }
        os << "]" << std::endl;
    };

    os << "Grammar{" << std::endl;
    os << sep(1) << "Properties{" << std::endl;
    for (const auto &prop: this->properties) {
        os << sep(2) << prop.name << " := " << prop.value;
        // if prop.name in REQUIRED_PROPS print (REQUIRED)
        if (std::find(std::begin(REQUIRED_PROPS), std::end(REQUIRED_PROPS), prop.name) != std::end(REQUIRED_PROPS)) {
            os << " (required)";
        } else {
            os << " (optional)";
        }
        os << std::endl;
    }
    os << sep(1) << "}" << std::endl;

    os << sep(1) << "Axiom{" << std::endl;
    os << sep(2) << "length := " << this->axiom.axiom.size() << std::endl;
    os << sep(2) << "axiom := " << this->axiom.axiom << std::endl;
    print_lookback(this->axiom.look_back);
    os << sep(1) << "}" << std::endl;

    os << sep(1) << "Productions{" << std::endl;
    for (const auto &prod: this->productions) {
        os << sep(2) << prod.predecessor << " -> " << prod.successor << std::endl;
        print_lookback(prod.look_back);
    }
    os << sep(1) << "}" << std::endl;
    os << "}" << std::endl;
}

PropertiesPtr Grammar::get_properties() {
    static PropertiesPtr _props;
    if (!_props) {
        _props = std::make_shared<Properties>();
        for (const auto &prop: this->properties) {
            _props->insert({prop.name, prop.value});
        }
    }
    return _props;
}

AxiomPtr Grammar::get_axiom() {
    static AxiomPtr _axiom;
    if (!_axiom) {
        _axiom = std::make_shared<Axiom>(this->axiom);
    }
    return _axiom;
}

ProductionsPtr Grammar::get_productions() {
    static ProductionsPtr _prods;
    if (!_prods) {
        _prods = std::make_shared<Productions>();
        for (const auto &prod: this->productions) {
            _prods->insert({prod.predecessor, {prod.successor, prod.look_back}});
        }
    }
    return _prods;
}

Grammar::Grammar(
        const std::vector<Property> &properties,
        Axiom axiom,
        const std::vector<Production> &productions
) : properties(properties), axiom(std::move(axiom)), productions(productions) {}

static size_t collect_size(Grammar &grammar, char *previous_result, size_t previous_size) {
    size_t result_size = 0;

    for (size_t i = 0; i < previous_size; i++) {
        auto production = grammar.get_production_successor(previous_result[i]);
        if (!production.has_value()) {
            result_size++;
            continue;
        }
        result_size += production->size();
    }

    return result_size;
}

static void collect_result(Grammar &grammar, char *previous_result, size_t previous_size, char *result) {
    size_t result_size = 0;

    for (size_t i = 0; i < previous_size; i++) {
        auto production = grammar.get_production_successor(previous_result[i]);
        if (!production.has_value()) {
            result[result_size++] = previous_result[i];
            continue;
        }
        std::memcpy(result + result_size, production->c_str(), production->size());
        result_size += production->size();
    }
}

std::string Grammar::cpu_produce() {
    size_t result_size = axiom.axiom.size();
    char *result = (char *) safe_calloc(result_size, sizeof(char));
    std::memcpy(result, axiom.axiom.c_str(), axiom.axiom.size());

    size_t steps = get_property_size_t("depth");
    for (size_t i = 0; i < steps; i++) {
        size_t new_size = collect_size(*this, result, result_size);
        char *new_result = (char *) safe_calloc(new_size, sizeof(char));
        collect_result(*this, result, result_size, new_result);
        safe_free(reinterpret_cast<void **>(&result));
        result = new_result;
        result_size = new_size;
    }

    std::string result_str(result, result_size);
    safe_free(reinterpret_cast<void **>(&result));
    return result_str;
}

float Grammar::get_property_float(const std::string &name) {
    // Defaults to 0
    auto it = get_properties()->find(name);
    if (it == get_properties()->end()) {
        return 0;
    }
    return it->second;
}

size_t Grammar::get_property_size_t(const std::string &name) {
    return static_cast<size_t>(get_property_float(name));
}

std::optional<std::tuple<std::string, LookBackTable>> Grammar::get_production(char predecessor) {
    auto it = get_productions()->find(predecessor);
    if (it == get_productions()->end()) {
        return {};
    }
    return it->second;
}

std::optional<std::string> Grammar::get_production_successor(char predecessor) {
    auto production = get_production(predecessor);
    if (!production.has_value()) {
        return {};
    }
    return std::get<0>(*production);
}

std::vector<int32_t> Axiom::get_as_opengl_data() const {
    std::vector<int32_t> data(axiom.size());
    for (size_t i = 0; i < axiom.size(); i++) {
        data[i] = (int) axiom[i];
    }
    return data;
}
