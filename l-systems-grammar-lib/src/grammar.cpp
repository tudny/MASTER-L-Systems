#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <utility>
#include <stack>
#include <numeric>
#include "grammar.h"
#include "memory_utils.h"
#include "Parser.H"
#include "Absyn.H"
#include "Skeleton.H"

const char STACK_OPENING_BRACKET = '[';
const char STACK_CLOSING_BRACKET = ']';
const char LEAF_OPENING_BRACKET = '{';
const char LEAF_CLOSING_BRACKET = '}';

constexpr const char *REQUIRED_PROPS[] = {"delta", "step", "depth"};

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

static std::vector<ColorT> make_linspace(std::vector<ColorT> colors, uint32_t number_of_colors) {
    if (colors.size() != 2) {
        throw std::runtime_error("Linspace colors must be set with exactly two colors");
    }
    if (number_of_colors < 2) {
        throw std::runtime_error("Number of colors must be at least 2");
    }
    auto start = colors[0];
    auto end = colors[1];
    std::vector<ColorT> linspace_colors;
    linspace_colors.reserve(number_of_colors);
    for (uint32_t i = 0; i < number_of_colors; ++i) {
        float t = static_cast<float>(i) / (number_of_colors - 1);
        ColorT color;
        for (size_t j = 0; j < 3; ++j) {
            color[j] = start[j] + t * (end[j] - start[j]);
        }
        color[3] = 1.0f; // Alpha channel
        linspace_colors.push_back(color);
    }
    return linspace_colors;
}

class GrammarBuilder {
public:
    GrammarBuilder() = default;

    void add_property(const std::string &name, float value) {
        properties.push_back({name, value});
    }

    void set_ignored(const std::string &_ignored) {
        this->ignored = _ignored;
    }

    void set_axiom(const std::string &_axiom) {
        this->axiom = {_axiom, compute_look_back(_axiom)};
    }

    void add_production(char predecessor,
                        const std::string &successor,
                        const std::string &left_context,
                        const std::string &right_context) {
        productions.push_back({predecessor, successor, left_context, right_context, compute_look_back(successor)});
    }

    void add_color(const ColorT &color) {
        colors.push_back(color);
    }

    GrammarPtr build() {
        for (const auto &prop: REQUIRED_PROPS) {
            if (std::find_if(properties.begin(), properties.end(), [&prop](const Property &p) {
                return p.name == prop;
            }) == properties.end()) {
                throw std::runtime_error("Missing required property: " + std::string(prop));
            }
        }

        return std::make_shared<Grammar>(properties, ignored, axiom, productions, colors);
    }

private:
    std::vector<Property> properties;
    std::string ignored;
    Axiom axiom;
    std::vector<Production> productions;
    std::vector<ColorT> colors;
};

/*
 * We define look_back table as the look-up for the tree nodes in the generated L-system word
 * For now we limit the size of the produced words to 2^31 - 1
 * Negative look_back indicates that the opening bracket is n steps before us
 * Positive look_back indicates that the closing bracket is n steps ahead of us
 * */
std::vector<int32_t> compute_look_back(const std::string &successor) {
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

static ColorT prod_to_color(Prod &value) {
    // Value must have 6 hex digits
    if (value.size() != 6) {
        throw std::runtime_error("Color value must have 6 hex digits: " + value);
    }
    ColorT color;
    for (size_t i = 0; i < 6; i += 2) {
        std::string hex = value.substr(i, 2);
        int32_t int_value = std::stoi(hex, nullptr, 16);
        color[i / 2] = static_cast<float>(int_value) / 255.0;
    }
    color[3] = 1.0f; // Alpha channel
    return color;
}

class ColorVisitor : public Skeleton {
public:
    void visitAColor(AColor *p) override {
        colors.push_back(prod_to_color(p->prod_));
    }

    void visitProd(Prod _number_of_colors) override {
        this->number_of_colors = std::stoi(_number_of_colors);
    }

    void produce_colors(GrammarBuilder &builder) {
        auto linspace_colors = make_linspace(colors, number_of_colors);
        for (const auto &color: linspace_colors) {
            builder.add_color(color);
        }
    }

private:
    std::vector<ColorT> colors;
    uint32_t number_of_colors;
};

class GrammarVisitor : public Skeleton {
public:
    void visitASTLProperty(ASTLProperty *p) override {
        builder.add_property(
                p->prod_1,
                std::stof(p->prod_2)
        );
    }

    void visitASTIgnore(ASTIgnore *p) override {
        builder.set_ignored(p->prod_);
    }

    void visitASTLAxiom(ASTLAxiom *p) override {
        builder.set_axiom(p->prod_);
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

    void visitAColor(AColor *p) override {
        auto value = p->prod_;
        try {
            auto color = prod_to_color(value);
            builder.add_color(color);
        } catch (...) {
            throw std::runtime_error("Invalid color value: " + value);
        }
    }

    void visitASTColorSpace(ASTColorSpace *p) override {
        ColorVisitor color_visitor;
        p->accept(&color_visitor);
        color_visitor.produce_colors(builder);
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
                right_context
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
    for (const auto &[prop_name, prop_value]: *this->properties) {
        os << sep(2) << prop_name << " := " << prop_value;
        // if prop.name in REQUIRED_PROPS print (REQUIRED)
        if (std::find(std::begin(REQUIRED_PROPS), std::end(REQUIRED_PROPS), prop_name) != std::end(REQUIRED_PROPS)) {
            os << " (required)";
        } else {
            os << " (optional)";
        }
        os << std::endl;
    }
    os << sep(1) << "}" << std::endl;
    os << sep(1) << "Ignored{" << std::endl;
    for (const auto &ig: this->ignored->ignored) {
        os << sep(2) << "char := " << (char) ig << std::endl;
    }
    os << sep(1) << "}" << std::endl;

    os << sep(1) << "Colors{" << std::endl;
    for (size_t i = 0; i < this->colors.size(); i++) {
        os << sep(2) << "color[" << i << "] := ";
        for (size_t j = 0; j < this->colors[i].size(); j++) {
            os << this->colors[i][j];
            if (j != this->colors[i].size() - 1) {
                os << ", ";
            }
        }
        os << std::endl;
    }
    os << sep(1) << "}" << std::endl;

    os << sep(1) << "Axiom{" << std::endl;
    os << sep(2) << "length := " << this->axiom->axiom.size() << std::endl;
    os << sep(2) << "axiom := " << this->axiom->axiom << std::endl;
    print_lookback(this->axiom->look_back);
    os << sep(1) << "}" << std::endl;

    os << sep(1) << "Productions{" << std::endl;
    for (const auto &prod: this->productions) {
        os << sep(2) << prod.predecessor << " -> " << prod.successor << std::endl;
        os << sep(2) << prod.left_context << " < . > " << prod.right_context << std::endl;
        print_lookback(prod.look_back);
    }
    os << sep(1) << "}" << std::endl;
    os << "}" << std::endl;
}

PropertiesPtr Grammar::get_properties() {
    return this->properties;
}

AxiomPtr Grammar::get_axiom() {
    return this->axiom;
}

static PropertiesPtr make_properties(const std::vector<Property> &properties) {
    auto props = std::make_shared<Properties>();
    for (const auto &property: properties) {
        (*props)[property.name] = property.value;
    }
    return props;
}

static IgnoredPtr make_ignored(const std::string &ignored) {
    std::set<char> ignored_set(ignored.begin(), ignored.end());
    return std::make_shared<Ignored>(std::vector<int32_t>(ignored_set.begin(), ignored_set.end()));
}

Grammar::Grammar(
        const std::vector<Property> &properties,
        const std::string &ignored,
        Axiom axiom,
        const std::vector<Production> &productions,
        const std::vector<ColorT> &colors
) : properties(make_properties(properties)),
    ignored(make_ignored(ignored)),
    axiom(std::make_shared<Axiom>(std::move(axiom))),
    productions(productions),
    colors(colors) {}

float Grammar::get_property_float(const std::string &name, float default_value) {
    auto it = get_properties()->find(name);
    if (it == get_properties()->end()) {
        return default_value;
    }
    return it->second;
}

size_t Grammar::get_property_size_t(const std::string &name, size_t default_value) {
    return static_cast<size_t>(get_property_float(name, static_cast<float>(default_value)));
}

static int32_t collect_size(const std::vector<Production> &productions, std::function<int32_t(Production)> sizer) {
    return std::accumulate(productions.begin(), productions.end(), 0,
                           [&sizer](int32_t size, const Production &prod) {
                               return size + sizer(prod);
                           });
}

static OpenGLReadyProductionDataType prepare_vector(size_t size) {
    return std::make_shared<std::vector<int32_t>>(size);
}

static OpenGLReadyProductionDataType prepare_vector(
        const std::vector<Production> &productions,
        const std::function<int32_t(Production)> &sizer
) {
    auto total_size = collect_size(productions, sizer);
    return prepare_vector(total_size);
}

static void copy_data_to_vector(
        std::vector<int32_t> &buffer,
        size_t offset,
        size_t size,
        const std::function<int32_t(size_t)> &source
) {
    for (size_t i = 0; i < size; i++) {
        buffer[offset + i] = source(i);
    }
}

template<typename T>
static void fill_vector(
        const std::vector<Production> &productions,
        std::function<const T *(const Production &)> &base,
        OpenGLReadyProductionDataType data,
        OpenGLReadyProductionDataType sizes = nullptr,
        OpenGLReadyProductionDataType offsets = nullptr
) {
    int32_t offset = 0;
    for (size_t i = 0; i < productions.size(); ++i) {
        auto &prod = productions[i];
        auto prop = base(prod);
        auto size = (int32_t) prop->size();
        if (sizes) (*sizes)[i] = size;
        if (offsets) (*offsets)[i] = offset;
        copy_data_to_vector(*data, offset, prop->size(), [&prop](size_t i) {
            return (int32_t) (*prop)[i];
        });
        offset += size;
    }
}

static
/* size, offset, data */
std::tuple<OpenGLReadyProductionDataType, OpenGLReadyProductionDataType, OpenGLReadyProductionDataType>
make_and_fill_vector(
        const std::vector<Production> &productions,
        std::function<const std::string *(const Production &)> base
) {
    OpenGLReadyProductionDataType sizes = prepare_vector(productions.size());
    OpenGLReadyProductionDataType offsets = prepare_vector(productions.size());
    OpenGLReadyProductionDataType data = prepare_vector(productions, [&base](auto prod) {
        return base(prod)->size();
    });

    fill_vector(productions, base, data, sizes, offsets);

    return {sizes, offsets, data};
}

OpenGLReadyProductions Grammar::get_opengl_ready_productions() const {
    auto predecessors = prepare_vector(productions, [](auto) { return 1; });
    copy_data_to_vector(*predecessors, 0, productions.size(), [this](size_t i) { return productions[i].predecessor; });
    auto look_back = prepare_vector(productions, [](auto p) { return p.look_back.size(); });
    std::function<const std::vector<int32_t> *(const Production &)> look_back_getter = [](const Production &p) {
        return &p.look_back;
    };
    fill_vector(productions, look_back_getter, look_back);
    auto [successors_sizes, successors_offsets, successors_data] = make_and_fill_vector(
            productions,
            [](const auto &p) {
                return &p.successor;
            });
    auto [left_context_sizes, left_context_offsets, left_context_data] = make_and_fill_vector(
            productions,
            [](const auto &p) {
                return &p.left_context;
            });
    auto [right_context_sizes, right_context_offsets, right_context_data] = make_and_fill_vector(
            productions,
            [](const auto &p) {
                return &p.right_context;
            });

    return {
            predecessors,
            look_back,
            successors_sizes,
            successors_offsets,
            successors_data,
            left_context_sizes,
            left_context_offsets,
            left_context_data,
            right_context_sizes,
            right_context_offsets,
            right_context_data,
            this->ignored->get_ignored_as_opengl_data()
    };
}

const std::vector<Production> &Grammar::get_raw_productions() const {
    return this->productions;
}

IgnoredPtr Grammar::get_ignored() {
    return this->ignored;
}

std::vector<ColorT> Grammar::get_colors() const {
    return colors;
}

std::vector<int32_t> Axiom::get_as_opengl_data() const {
    std::vector<int32_t> data(axiom.size());
    for (size_t i = 0; i < axiom.size(); i++) {
        data[i] = (int) axiom[i];
    }
    return data;
}
