#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace lang::ast::cypher
{

struct TranslatorContext
{
    std::unordered_set<std::string> variableTable;
    std::unordered_set<std::string> functionTable;
    std::uint32_t quantifierLevel = 0;
    std::vector<std::string> returns;
    bool exceptRule = false;
};

}; // namespace lang::ast::cypher