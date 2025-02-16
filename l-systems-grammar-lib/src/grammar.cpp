
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <utility>
#include "grammar.h"
#include "str_utils.h"

constexpr const char *ALLOWED_OPERATORS = "+-&^/\\|Ff[]";
constexpr const char *REQUIRED_PROPS[] = {"delta", "step", "depth"};

constexpr const char *PROPERTY_SEPARATOR = ":=";
constexpr const char *PRODUCTION_SEPARATOR = "->";

class GrammarBuilder {
public:
    GrammarBuilder() = default;

    void add_property(const std::string &name, float value) {
        properties.push_back({name, value});
    }

    void set_axiom(const std::string &_axiom) {
        this->axiom = {_axiom};
    }

    void add_production(char predecessor, const std::string &successor) {
        productions.push_back({predecessor, successor});
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
                builder.set_axiom(line);
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
                builder.add_production(predecessor[0], successor);
                break;
            }
        }
    }

    return builder.build();
}

void Grammar::print() {
    std::cout << "Properties:" << std::endl;
    for (const auto &prop: this->properties) {
        std::cout << prop.name << " := " << prop.value << std::endl;
    }

    std::cout << "Axiom: " << this->axiom.axiom << std::endl;

    std::cout << "Productions:" << std::endl;
    for (const auto &prod: this->productions) {
        std::cout << prod.predecessor << " -> " << prod.successor << std::endl;
    }
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
            _prods->insert({prod.predecessor, prod.successor});
        }
    }
    return _prods;
}

Grammar::Grammar(
        const std::vector<Property> &properties,
        Axiom axiom,
        const std::vector<Production> &productions
) : properties(properties), axiom(std::move(axiom)), productions(productions) {}
