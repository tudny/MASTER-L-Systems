#ifndef LSYSTEMS_GRAMMAR_H
#define LSYSTEMS_GRAMMAR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <set>

using LookBackTable = std::vector<int32_t>;

class Property {
public:
    std::string name;
    float value;
};

using Properties = std::unordered_map<decltype(Property::name), decltype(Property::value)>;
using PropertiesPtr = std::shared_ptr<Properties>;

class Ignored {
public:
    [[nodiscard]] std::shared_ptr<std::vector<int32_t>> get_ignored_as_opengl_data() const {
        return std::make_shared<std::vector<int32_t>>(ignored);
    }

    std::vector<int32_t> ignored;
};

using IgnoredPtr = std::shared_ptr<Ignored>;

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
    std::string left_context;
    std::string right_context;
    LookBackTable look_back;

    bool operator<(const Production &other) const {
        if (predecessor != other.predecessor) {
            return predecessor < other.predecessor;
        }
        if (successor != other.successor) {
            return successor < other.successor;
        }
        if (left_context != other.left_context) {
            return left_context < other.left_context;
        }
        return right_context < other.right_context;
        // look back is a derivative of the successor
    }
};

using OpenGLReadyProductionDataType = const std::shared_ptr<std::vector<int32_t>>;

#define PRODUCTION_DATA(X) \
    OpenGLReadyProductionDataType X ## _sizes; \
    OpenGLReadyProductionDataType X ## _offsets; \
    OpenGLReadyProductionDataType X ## _data;

class OpenGLReadyProductions {
public:
    OpenGLReadyProductionDataType predecessors;
    OpenGLReadyProductionDataType look_back;
    PRODUCTION_DATA(successors)
    PRODUCTION_DATA(left_context)
    PRODUCTION_DATA(right_context)
    OpenGLReadyProductionDataType ignored;
};

class Grammar {
public:
    PropertiesPtr get_properties();

    IgnoredPtr get_ignored();

    AxiomPtr get_axiom();

    [[nodiscard]] const std::vector<Production> &get_raw_productions() const;

    float get_property_float(const std::string &name);

    size_t get_property_size_t(const std::string &name);

    void print(std::ostream &os = std::cout);

    [[nodiscard]] OpenGLReadyProductions get_opengl_ready_productions() const;

    Grammar(
            const std::vector<Property> &properties,
            const std::string &ignored,
            Axiom axiom,
            const std::vector<Production> &productions
    );

private:
    PropertiesPtr properties;
    IgnoredPtr ignored;
    AxiomPtr axiom;
    std::vector<Production> productions;
};

using GrammarPtr = std::shared_ptr<Grammar>;

GrammarPtr load_grammar(const std::string &path);

std::vector<int32_t> compute_look_back(const std::string &successor);

#endif //LSYSTEMS_GRAMMAR_H
