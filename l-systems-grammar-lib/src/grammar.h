#ifndef LSYSTEMS_GRAMMAR_H
#define LSYSTEMS_GRAMMAR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

extern const char *ALLOWED_OPERATORS;

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
};

using AxiomPtr = std::shared_ptr<Axiom>;

class Production {
public:
    char predecessor;
    std::string successor;
};

using Productions = std::unordered_map<decltype(Production::predecessor), decltype(Production::successor)>;
using ProductionsPtr = std::shared_ptr<Productions>;

class Grammar {
public:
    PropertiesPtr get_properties();
    AxiomPtr get_axiom();
    ProductionsPtr get_productions();

    float get_property_float(const std::string &name);
    size_t get_property_size_t(const std::string &name);

    std::optional<std::string> get_production(char predecessor);

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
