#pragma once

#include <stack>
#include <string_view>
#include <unordered_set>

namespace cypher
{

struct SymbolTables
{
    std::unordered_set<std::string> variables;
    std::unordered_set<std::string> functions;
};

using ContextContainer = std::stack<std::unordered_set<std::string_view>>;

struct Context
{
    ContextContainer sets;
};

}; // namespace cypher