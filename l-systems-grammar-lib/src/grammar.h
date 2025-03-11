#ifndef LSYSTEMS_GRAMMAR_H
#define LSYSTEMS_GRAMMAR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

extern const char *ALLOWED_OPERATORS;

using LookBackTable = std::vector<int32_t>;

class Property {
public:
    std::string name;
    float value;
};

using Properties = std::unordered_map<decltype(Property::name), decltype(Property::value)>;
using PropertiesPtr = std::shared_ptr<Properties>;

class Axiom {
public:
    std::string axiom;
    LookBackTable look_back;

    [[nodiscard]] std::vector<int32_t> get_as_opengl_data() const;
};

using AxiomPtr = std::shared_ptr<Axiom>;

class Production {
public:
    char predecessor;
    std::string successor;
    LookBackTable look_back;
};

using Productions = std::unordered_map<decltype(Production::predecessor), std::tuple<decltype(Production::successor), decltype(Production::look_back)>>;
using ProductionsPtr = std::shared_ptr<Productions>;

class Grammar {
public:
    PropertiesPtr get_properties();
    AxiomPtr get_axiom();
    ProductionsPtr get_productions();

    float get_property_float(const std::string &name);
    size_t get_property_size_t(const std::string &name);

    std::optional<std::tuple<std::string, LookBackTable>> get_production(char predecessor);
    std::optional<std::string> get_production_successor(char predecessor);

    void print(std::ostream &os = std::cout);

    std::string cpu_produce();

    Grammar(const std::vector<Property> &properties, Axiom axiom, const std::vector<Production> &productions);

private:
    std::vector<Property> properties;
    Axiom axiom;
    std::vector<Production> productions;
};

using GrammarPtr = std::shared_ptr<Grammar>;

GrammarPtr load_grammar(const std::string &path);

#endif //LSYSTEMS_GRAMMAR_H
