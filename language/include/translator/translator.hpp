#pragma once

#include <string>
#include <translator/format.hpp>

#include <lang/ast.hpp>

#include <fmt/format.h>

#include <magic_enum/magic_enum.hpp>

#include <sstream>
#include <stdexcept>
namespace cypher
{

class Translator
{
public:
    static std::string translateRule(const lang::ast::Rule& rule)
    {
        std::ostringstream oss;
        oss << fmt::format(RuleNameFormat, rule.name) << "\n";
        if(rule.description.has_value())
        {
            oss << fmt::format(DescriptionFormat, *rule.description) << "\n";
        }
        oss << fmt::format(PriorityFormat, magic_enum::enum_name(rule.priority)) << "\n";
        return oss.str();
    }

    static std::string translateBlock(const lang::ast::Block& block)
    {
        std::ostringstream oss;
        
        for (const auto& stmtPtr : block.statements)
        {
            if (!stmtPtr) continue;
            oss << translateStatement(*stmtPtr);
            oss << "\n";
        }

        return oss.str();
    }

    static std::string translateStatement(const lang::ast::Statement& statement)
    {
        return std::visit([](auto&& argPtr) -> std::string {
            using T = std::decay_t<decltype(argPtr)>;

            if (!argPtr)
                return std::string(InDevelopmentFormat);

            if constexpr (std::is_same_v<T, lang::ast::AssignmentStatementPtr>)
            {
                return std::string(InDevelopmentFormat);
            }
            else if constexpr (std::is_same_v<T, lang::ast::QuantifierStatementPtr>)
            {
                return std::string(InDevelopmentFormat);
            }
            else if constexpr (std::is_same_v<T, lang::ast::SelectionStatementPtr>)
            {
                return std::string(InDevelopmentFormat);
            }
            else if constexpr (std::is_same_v<T, lang::ast::СonditionalStatementPtr>)
            {
                return std::string(InDevelopmentFormat);
            }
            else if constexpr (std::is_same_v<T, lang::ast::ExceptStatementPtr>)
            {
                return std::string(InDevelopmentFormat);
            }
            else
            {
                throw std::runtime_error("Unknown Statement type in translateStatement");
            }
        }, 
        statement);
    }

    static std::string translateQuantifierStatement(const lang::ast::QuantifierStatement& quant)
    {
        // TODO: WITH collect(s) AS systems - implement aggregation
        // TODO: ALL, ANY - implement base quantifier
        // TODO: ExperssionPtr - implement base translator
        if(quant.type == lang::ast::QuantifierType::All)
        {
           
        }
        else
        {
        
        }
        return "";
    }
};
};