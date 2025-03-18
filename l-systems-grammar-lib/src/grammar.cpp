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

const char STACK_OPENING_BRACKET = '[';
const char STACK_CLOSING_BRACKET = ']';

const char *ALLOWED_OPERATORS = "+-&^/\\|Ff[]";
constexpr const char *REQUIRED_PROPS[] = {"delta", "step", "depth"};

constexpr const char *PROPERTY_SEPARATOR = ":=";
constexpr const char *PRODUCTION_SEPARATOR = "->";

/*
 * We define look_back table as the look-up for the tree nodes in the generated L-system word
 * For now we limit the size of the produced words to 2^31 - 1
 * Negative look_back indicates that the opening bracket is n steps before us
 * Positive look_back indicates that the closing bracket is n steps ahead of us
 * */
static std::vector<int32_t> compute_look_back(std::string &successor) {
    std::vector<int32_t> look_back(successor.size(), 0);
    std::stack<int32_t> stack;

    for (int32_t i = 0; i < static_cast<int32_t>(successor.size()); i++) {
        if (successor[i] == STACK_OPENING_BRACKET) {
            stack.push(i);
        } else if (successor[i] == STACK_CLOSING_BRACKET) {
            if (stack.empty()) {
                throw std::runtime_error("Unbalanced brackets in successor: " + successor);
            }
            auto matching_open_bracket = stack.top();
            stack.pop();
            look_back[i] = matching_open_bracket - i;
            look_back[matching_open_bracket] = i - matching_open_bracket;
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

    void add_production(char predecessor, const std::string &successor, LookBackTable look_back) {
        productions.push_back({predecessor, successor, std::move(look_back)});
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

enum ParseState {
    PROPERTY,
    AXIOM,
    PRODUCTION
};

// if line begins with "--" it moves to next state
// if line begins with "#" it is just a comment
// props are passed as:
// property_name := value
// axiom is just a word over [a-zA-Z]
// productions are passed as:
// <char> -> <string>
// productions are words over [a-zA-Z] and operators
// last value of axiom and productions is taken

// This grammar should be accepted

//delta:=90
//step:=100
//depth:=7
//--
//L
//--
//L -> +RF−LFL−FR+
//R -> −LF+RFR+FL−
GrammarPtr load_grammar(const std::string &path) {
    GrammarBuilder builder{};

    std::string line;
    ParseState state = PROPERTY;
    std::ifstream file(path);

    if (!file.is_open()) {
        auto workdir = std::filesystem::current_path();
        throw std::runtime_error("Failed to open file: " + path + " in " + workdir.string());
    }

    while (std::getline(file, line)) {
        // Line can be ignored, as it is a comment, or is empty
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Line is a state switcher
        if (line[0] == '-' && line[1] == '-') {
            switch (state) {
                case PROPERTY:
                    state = AXIOM;
                    break;
                case AXIOM:
                    state = PRODUCTION;
                    break;
                case PRODUCTION:
                    // Productions cannot change the builder state
                    throw std::runtime_error("Unexpected '--' in productions");
            }
            continue;
        }

        switch (state) {
            case PROPERTY: {
                auto pos = line.find(PROPERTY_SEPARATOR);
                if (pos == std::string::npos) {
                    throw std::runtime_error("Invalid property definition: " + line);
                }

                std::string name = trim(line.substr(0, pos));
                float value = std::stof(line.substr(pos + strlen(PROPERTY_SEPARATOR)));
                builder.add_property(name, value);
                break;
            }
            case AXIOM:
                builder.set_axiom(line, compute_look_back(line));
                break;
            case PRODUCTION: {
                auto pos = line.find(PRODUCTION_SEPARATOR);
                if (pos == std::string::npos) {
                    throw std::runtime_error("Invalid production definition: " + line);
                }

                std::string predecessor = trim(line.substr(0, pos));
                if (predecessor.size() != 1) {
                    throw std::runtime_error("Predecessor must be a single char, but got: " + predecessor);
                }
                std::string successor = trim(line.substr(pos + strlen(PRODUCTION_SEPARATOR)));
                auto look_back = compute_look_back(successor);
                builder.add_production(predecessor[0], successor, look_back);
                break;
            }
        }
    }

    return builder.build();
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
