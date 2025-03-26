#pragma once

#include "constant.hpp"
#include "context.hpp"
#include "translator/format.hpp"

#include <algorithm>
#include <lang/expression.hpp>
#include <ranges>
#include <vector>

namespace cypher
{
inline bool LiteralInSuperSet(const lang::ast::VariablePtr &lit)
{
    return superSets.contains(lit->name);
}

inline bool LiteralInContext(const Context &context, const lang::ast::VariablePtr &lit)
{
    return context.sets.empty() ? false : context.sets.top().contains(lit->name);
}

inline bool FuncCallEqualName(const lang::ast::CallPtr &funcCall, std::string_view name)
{
    return funcCall->functionName == name;
}

inline std::string CreateMatchForCall(const std::vector<std::string> &names)
{
    std::string result{withSelectionFormat};
    std::ranges::for_each(names, [&result](const auto &name) { result += name + ", "; });
    return result;
}

}; // namespace cypher