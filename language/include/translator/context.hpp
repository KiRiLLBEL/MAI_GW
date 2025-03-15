#pragma once

#include <stack>
#include <string_view>
#include <unordered_set>

namespace cypher {

using ContextContainer = std::stack<std::unordered_set<std::string_view>>;

struct Context
{
    ContextContainer sets;
};

};