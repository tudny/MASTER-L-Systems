#include <string>
#include <ranges>

// Taken from https://stackoverflow.com/a/66897681/7095554

std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };

    // erase the the spaces at the back first
    // so we don't have to do extra work
    s.erase(
            std::ranges::find_if(s | std::views::reverse, not_space).base(),
            s.end());

    // erase the spaces at the front
    s.erase(
            s.begin(),
            std::ranges::find_if(s, not_space));

    return s;
}
