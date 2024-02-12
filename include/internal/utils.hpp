#pragma once

#include <string>
#include <vector>

namespace jsonpp::utils
{

    // Define a helper type to combine lambdas into a single visitor object.
    template <class... Ts>
    struct inline_visitor : Ts...
    {
        using Ts::operator()...;
    };
    // Explicit deduction guide (not needed as of C++20)
    template <class... Ts>
    inline_visitor(Ts...) -> inline_visitor<Ts...>;

    std::string join(std::vector<std::string> &elems, char delimiter);

}
