#pragma once

#include "constant.hpp"
#include "context.hpp"

#include <lang/expression.hpp>

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

inline bool FuncCallEqualName(const lang::ast::FunctionCallPtr &funcCall, std::string_view name)
{
    return funcCall->functionName == name;
}
}; // namespace cypher