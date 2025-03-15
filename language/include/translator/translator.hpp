#pragma once

#include "translator/constant.hpp"
#include <lang/expression.hpp>
#include <translator/context.hpp>
#include <string>
#include <translator/helpers.hpp>
#include <translator/format.hpp>

#include <lang/ast.hpp>

#include <fmt/format.h>

#include <magic_enum/magic_enum.hpp>

#include <sstream>
#include <stdexcept>
#include <type_traits>
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
        Context context;
        return oss.str();
    }

    static std::string translateBlock(Context& context, const lang::ast::Block& block)
    {
        std::ostringstream oss;
        
        for (const auto& stmtPtr : block.statements)
        {
            if (!stmtPtr) continue;
            oss << translateStatement(context, *stmtPtr);
            oss << "\n";
        }

        return oss.str();
    }

    static std::string translateStatement(Context& context, const lang::ast::Statement& statement)
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

    template<lang::ast::QuantifierType quant>
    static std::string translateSelectionStatement(Context& context, const lang::ast::SelectionStatement& stmt)
    {
        std::string result;
        std::string selectionFrom;
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, lang::ast::VariablePtr>)
            {
                if (not LiteralInSuperSet(arg) and not LiteralInContext(context, arg))
                {
                    throw std::runtime_error(fmt::format(SuperSetsContainsError, arg->name));
                }
                else if(LiteralInSuperSet(arg))
                {
                    result = fmt::format(MatchFormat, arg->name) + "\n" + fmt::format(WithFormat, arg->name) + "\n";
                }
                selectionFrom = arg->name;
            }
            if constexpr (std::is_same_v<T, lang::ast::FunctionCallPtr>)
            {
                if (not FuncCallEqualName(arg, RouteFunc))
                {
                    throw std::runtime_error(fmt::format(SelectionFromRouteError));
                }
                // TODO: add selectionFrom for route
            }
        }, *stmt.collectionExpr);
        context.sets.push(std::unordered_set<std::string_view>());
        auto buildSelection = [&](auto&& self, const std::vector<std::string>& names, size_t index) -> std::string {
            context.sets.top().insert(names[index]);
            if (index == names.size() - 1)
            {
                return fmt::format(QuantFormat, magic_enum::enum_name(quant), names[index], selectionFrom, "ok");
            }
            else
            {
                return fmt::format(QuantFormat, magic_enum::enum_name(quant), names[index], selectionFrom, self(self, names, index + 1));
            }
        };
        return result + fmt::format(WhereFormat, buildSelection(buildSelection, stmt.varNames, 0));
    }
    
    static std::string translateQuantifierStatement(const lang::ast::QuantifierStatement& stmt)
    {
        // TODO: ExperssionPtr - implement base translator
        Context context;
        switch (stmt.type)
        {
            case lang::ast::QuantifierType::ALL:
                return translateSelectionStatement<lang::ast::QuantifierType::ALL>(context, *stmt.body);
            case lang::ast::QuantifierType::ANY:
                return translateSelectionStatement<lang::ast::QuantifierType::ANY>(context, *stmt.body);
            default:
                throw std::runtime_error("Неизвестный тип квантора");
        }
    }
};
};