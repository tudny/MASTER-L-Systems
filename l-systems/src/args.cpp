#include "args.hpp"

void Context::validate_grammar_path() {
    if (grammar_path.empty()) {
        throw std::runtime_error("Grammar path cannot be empty");
    }
    if (grammar_path.find(".ls") == std::string::npos) {
        throw std::runtime_error("Grammar path must end with '.ls'");
    }
    if (!std::filesystem::exists(grammar_path)) {
        throw std::runtime_error("Grammar file does not exist");
    }
}

void Context::print(std::ostream &ostream) const {
    ostream << "Context{" << std::endl;
    ostream << "  grammar_path: " << grammar_path << std::endl;
    ostream << "  working_directory: " << working_directory << std::endl;
    ostream << "}" << std::endl;
}

ContextPtr parse_args(int argc, char **argv) {
    auto working_directory = std::filesystem::current_path();

    args::ArgumentParser parser("L-Systems", "L-Systems application");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Positional<std::string> grammar_path(parser, "grammar", "Path to grammar file", "sample/demo-grammar.ls");

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help&) {
        std::cout << parser;
        exit(0);
    } catch (args::ParseError &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        exit(1);
    } catch (args::ValidationError &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        exit(1);
    }

    auto context = std::make_shared<Context>(grammar_path.Get(), working_directory);

    context->print();

    return context;
}
