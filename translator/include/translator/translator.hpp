#pragma once

#include <sstream>
#include <lang/ast.hpp>
#include <magic_enum/magic_enum.hpp>
namespace cypher
{

class Translator
{
public:
    static std::string translateRule(const lang::ast::Rule& rule)
    {
        std::ostringstream oss;
        oss << "// [RULE]: " << rule.name << "\n";
        if(rule.description.has_value())
        {
            oss << "// [DESCRIPTION]: " << *rule.description << "\n";
        }
        oss << "// [PRIORITY]: " << magic_enum::enum_name(rule.priority) << "\n";
        return oss.str();
    }
};
};