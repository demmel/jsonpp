#include "utils.hpp"

std::string join(std::vector<std::string> &elems, char delimiter)
{

    size_t needed = elems.size() - 1; // For the delimiters
    for (auto &e : elems)
    {
        needed += e.size(); // For each string
    }

    std::string s;
    s.reserve(needed);

    for (size_t i = 0; i < elems.size(); ++i)
    {
        auto e = elems[i];
        s.append(e);
        if (i != elems.size() - 1)
        {
            s.push_back(delimiter);
        }
    }

    return s;
}