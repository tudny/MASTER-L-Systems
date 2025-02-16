#ifndef LSYSTEMS_GRAMMAR_H
#define LSYSTEMS_GRAMMAR_H

#include <string>
#include <vector>
#include <unordered_map>

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
    void print();

    Grammar(const std::vector<Property> &properties, Axiom axiom, const std::vector<Production> &productions);

private:
    std::vector<Property> properties;
    Axiom axiom;
    std::vector<Production> productions;
};

using GrammarPtr = std::shared_ptr<Grammar>;

GrammarPtr load_grammar(const std::string &path);

#endif //LSYSTEMS_GRAMMAR_H
