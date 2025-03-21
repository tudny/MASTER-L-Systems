#ifndef LSYSTEMS_ARGS_HPP
#define LSYSTEMS_ARGS_HPP

#include <memory>
#include "args.hxx"
#include <filesystem>

class Context {
public:
    explicit Context(
            std::string grammar_path,
            std::string working_directory
    ) :
            grammar_path(std::move(grammar_path)),
            working_directory(std::move(working_directory)) {
        validate_grammar_path();
    }

    void print(std::ostream &ostream = std::cout) const;

    const std::string grammar_path;
    const std::string working_directory;

private:

    void validate_grammar_path();
};

using ContextPtr = std::shared_ptr<Context>;

ContextPtr parse_args(int argc, char *argv[]);

#endif //LSYSTEMS_ARGS_HPP
