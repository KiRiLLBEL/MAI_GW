#pragma once

#include "fmt/base.h"
#include "translator/constant.hpp"
#include <compare>
#include <cstdint>
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
#include <variant>
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
        return std::visit([&](auto&& argPtr) -> std::string {
            using T = std::decay_t<decltype(argPtr)>;

            if (!argPtr)
                return fmt::to_string(InDevelopmentFormat);

            if constexpr (std::is_same_v<T, lang::ast::AssignmentStatementPtr>)
            {
                return translateAssignmentStatement(context, argPtr);
            }
            else if constexpr (std::is_same_v<T, lang::ast::QuantifierStatementPtr>)
            {
                return translateQuantifierStatement(context, argPtr);
            }
            else if constexpr (std::is_same_v<T, lang::ast::СonditionalStatementPtr>)
            {
                return fmt::to_string(InDevelopmentFormat);
            }
            else if constexpr (std::is_same_v<T, lang::ast::ExceptStatementPtr>)
            {
                return fmt::to_string(InDevelopmentFormat);
            }
            else
            {
                throw std::runtime_error("Unknown Statement type in translateStatement");
            }
        }, 
        statement);
    }
    static constexpr auto inVisitor(auto&& arg) -> std::string
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, lang::ast::LiteralPtr>)
        {
            std::visit([&](auto&& ptr) -> std::string
            {
                using M = std::decay_t<decltype(ptr)>;
                if constexpr (not std::is_same_v<M, std::vector<lang::ast::ExpressionPtr>>)
                {
                    return fmt::to_string(ptr);
                }
                else
                {
                    return "";
                }
            }, arg->value);
        }
        else if constexpr (std::is_same_v<T, lang::ast::VariablePtr>)
        {
            return arg->name;
        }
        else if constexpr (std::is_same_v<T, lang::ast::AccessExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), translateExpression(arg->operand), arg->prop);
        }
        else if constexpr (std::is_same_v<T, lang::ast::UnaryExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), translateExpression(arg->operand));
        }
        else if constexpr (std::is_same_v<T, lang::ast::BinaryExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), translateExpression(arg->left), translateExpression(arg->right));
        }
        else if constexpr (std::is_same_v<T, lang::ast::TernaryExprPtr>)
        {
            return fmt::to_string(InDevelopmentFormat);
        }
        else if constexpr (std::is_same_v<T, lang::ast::FunctionCallPtr>)
        {
            return fmt::to_string(InDevelopmentFormat);
        }
        return "";
    };
    static std::string translateExpression(const lang::ast::ExpressionPtr& expr)
    {
        return std::visit([&](auto&& arg)
        {
            return inVisitor(arg);
        }, *expr);
    }

    template<lang::ast::QuantifierType quant>
    static std::string translateSelectionStatement(Context& context, const lang::ast::SelectionStatementPtr& stmt)
    {
        std::string result;
        std::string selectionFrom;
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, lang::ast::VariablePtr>)
            {
                if(context.sets.empty())
                {
                    if (not LiteralInSuperSet(arg))
                    {
                        throw std::runtime_error(fmt::format(SuperSetsContainsError, arg->name));
                    }
                    result = fmt::format(MatchFormat, arg->name) + "\n" + fmt::format(WithCollectFormat, arg->name) + "\n";
                }
                else
                {
                    if(not LiteralInContext(context, arg))
                    {
                        throw std::runtime_error(fmt::format(SuperSetsContainsError, arg->name));
                    }
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
        }, *stmt->collectionExpr);
        context.sets.push(std::unordered_set<std::string_view>());
        auto buildSelection = [&](auto&& self, const std::vector<std::string>& names, size_t index) -> std::string
        {
            context.sets.top().insert(names[index]);
            if (index == names.size() - 1)
            {
                return std::visit([&](auto&& arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                
                    if constexpr (std::is_same_v<T, lang::ast::ExpressionPtr>)
                    {
                        return fmt::format(QuantFormat, magic_enum::enum_name(quant), names[index], selectionFrom, translateExpression(arg));
                    }
                    return fmt::to_string(InDevelopmentFormat);
                }, stmt->body);
            }
            else
            {
                return fmt::format(QuantFormat, magic_enum::enum_name(quant), names[index], selectionFrom, self(self, names, index + 1));
            }
        };
        return result + fmt::format(WhereFormat, buildSelection(buildSelection, stmt->varNames, 0));
    }

    static std::string translateAssignmentStatement(Context& context, const lang::ast::AssignmentStatementPtr& stmt)
    {
        std::string result;
        std::visit([&](auto&& ptr)
        {
            using T = std::decay_t<decltype(ptr)>;
            if constexpr (std::is_same_v<T, lang::ast::LiteralPtr>)
            {
                std::visit([&](auto&& value)
                {
                    using M = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<M, std::string> or std::is_same_v<M, std::int64_t> or std::is_same_v<M, bool>)
                    {
                        result = fmt::format(WithVariableFormat, value, stmt->name);
                    }
                    // TODO: Add set translator
                }, ptr->value);
            }
        }, *stmt->valueExpr);
        return result;
    }
    
    static std::string translateQuantifierStatement(Context& context, const lang::ast::QuantifierStatementPtr& stmt)
    {
        // TODO: ExperssionPtr - implement base translator
        switch (stmt->type)
        {
            case lang::ast::QuantifierType::ALL:
                return translateSelectionStatement<lang::ast::QuantifierType::ALL>(context, stmt->body);
            case lang::ast::QuantifierType::ANY:
                return translateSelectionStatement<lang::ast::QuantifierType::ANY>(context, stmt->body);
            default:
                throw std::runtime_error("Неизвестный тип квантора");
        }
    }
};
};