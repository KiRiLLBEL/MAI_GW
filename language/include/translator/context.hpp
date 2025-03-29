#pragma once

#include "ast/expression.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lang::ast::cypher
{

struct TranslatorContext
{
    std::unordered_set<std::string> variableTable;
    std::unordered_set<std::string> functionTable;
    std::unordered_map<std::string, KeywordSets> variableType;
    std::uint32_t quantifierLevel = 0;
    std::vector<std::string> returns;
    bool exceptRule = false;
};

}; // namespace lang::ast::cypher