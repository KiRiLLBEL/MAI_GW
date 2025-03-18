#pragma once

#include <translator/constant.hpp>
#include <translator/context.hpp>
#include <translator/format.hpp>
#include <translator/helpers.hpp>

#include <lang/ast.hpp>
#include <lang/expression.hpp>

#include <fmt/base.h>
#include <fmt/format.h>

#include <magic_enum/magic_enum.hpp>

#include <compare>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
namespace cypher
{

class Translator
{
public:
    static std::string TranslateRule(const lang::ast::Rule &rule)
    {
        std::ostringstream oss;
        oss << fmt::format(ruleNameFormat, rule.name) << "\n";
        if (rule.description.has_value())
        {
            oss << fmt::format(descriptionFormat, *rule.description) << "\n";
        }
        oss << fmt::format(priorityFormat, magic_enum::enum_name(rule.priority)) << "\n";
        Context context;
        return oss.str();
    }

    static std::string TranslateBlock(Context &context, const lang::ast::Block &block)
    {
        std::ostringstream oss;

        for (const auto &stmtPtr : block.statements)
        {
            if (!stmtPtr)
            {
                continue;
            }
            oss << TranslateStatement(context, *stmtPtr);
            oss << "\n";
        }

        return oss.str();
    }

    static std::string TranslateStatement(Context &context, const lang::ast::Statement &statement)
    {
        return std::visit(
            [&](auto &&argPtr) -> std::string
            {
                using T = std::decay_t<decltype(argPtr)>;

                if (!argPtr)
                {
                    return fmt::to_string(inDevelopmentFormat);
                }

                if constexpr (std::is_same_v<T, lang::ast::AssignmentStatementPtr>)
                {
                    return TranslateAssignmentStatement(context, argPtr);
                }
                else if constexpr (std::is_same_v<T, lang::ast::QuantifierStatementPtr>)
                {
                    return TranslateQuantifierStatement(context, argPtr);
                }
                else if constexpr (std::is_same_v<T, lang::ast::СonditionalStatementPtr>)
                {
                    return fmt::to_string(inDevelopmentFormat);
                }
                else if constexpr (std::is_same_v<T, lang::ast::ExceptStatementPtr>)
                {
                    return fmt::to_string(inDevelopmentFormat);
                }
                else
                {
                    throw std::runtime_error("Unknown Statement type in translateStatement");
                }
            },
            statement);
    }
    static constexpr auto InVisitor(auto &&arg) -> std::string
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, lang::ast::LiteralPtr>)
        {
            std::visit(
                [&](auto &&ptr) -> std::string
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
                },
                arg->value);
        }
        else if constexpr (std::is_same_v<T, lang::ast::VariablePtr>)
        {
            return arg->name;
        }
        else if constexpr (std::is_same_v<T, lang::ast::AccessExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)),
                               TranslateExpression(arg->operand), arg->prop);
        }
        else if constexpr (std::is_same_v<T, lang::ast::UnaryExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)),
                               TranslateExpression(arg->operand));
        }
        else if constexpr (std::is_same_v<T, lang::ast::BinaryExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), TranslateExpression(arg->left),
                               TranslateExpression(arg->right));
        }
        else if constexpr (std::is_same_v<T, lang::ast::TernaryExprPtr>)
        {
            return fmt::to_string(inDevelopmentFormat);
        }
        else if constexpr (std::is_same_v<T, lang::ast::FunctionCallPtr>)
        {
            return fmt::to_string(inDevelopmentFormat);
        }
        return "";
    };
    static std::string TranslateExpression(const lang::ast::ExpressionPtr &expr)
    {
        return std::visit([&](auto &&arg) { return InVisitor(arg); }, *expr);
    }

    template <lang::ast::QuantifierType quant>
    static std::string TranslateSelectionStatement(Context &context,
                                                   const lang::ast::SelectionStatementPtr &stmt)
    {
        std::string result;
        std::string selectionFrom;
        std::visit(
            [&](auto &&arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, lang::ast::VariablePtr>)
                {
                    if (context.sets.empty())
                    {
                        if (not LiteralInSuperSet(arg))
                        {
                            throw std::runtime_error(
                                fmt::format(superSetsContainsError, arg->name));
                        }
                        result = fmt::format(matchFormat, arg->name) + "\n" +
                                 fmt::format(withCollectFormat, arg->name) + "\n";
                    }
                    else
                    {
                        if (not LiteralInContext(context, arg))
                        {
                            throw std::runtime_error(
                                fmt::format(superSetsContainsError, arg->name));
                        }
                    }
                    selectionFrom = arg->name;
                }
                if constexpr (std::is_same_v<T, lang::ast::FunctionCallPtr>)
                {
                    if (not FuncCallEqualName(arg, routeFunc))
                    {
                        throw std::runtime_error(fmt::format(selectionFromRouteError));
                    }
                    // TODO: add selectionFrom for route
                }
            },
            *stmt->collectionExpr);
        context.sets.emplace();
        auto buildSelection = [&](auto &&self, const std::vector<std::string> &names,
                                  size_t index) -> std::string
        {
            context.sets.top().insert(names[index]);
            if (index == names.size() - 1)
            {
                return std::visit(
                    [&](auto &&arg) -> std::string
                    {
                        using T = std::decay_t<decltype(arg)>;

                        if constexpr (std::is_same_v<T, lang::ast::ExpressionPtr>)
                        {
                            return fmt::format(quantFormat, magic_enum::enum_name(quant),
                                               names[index], selectionFrom,
                                               TranslateExpression(arg));
                        }
                        return fmt::to_string(inDevelopmentFormat);
                    },
                    stmt->body);
            }

            return fmt::format(quantFormat, magic_enum::enum_name(quant), names[index],
                               selectionFrom, self(self, names, index + 1));
        };
        return result + fmt::format(whereFormat, buildSelection(buildSelection, stmt->varNames, 0));
    }

    static std::string TranslateAssignmentStatement(Context & /*unused*/,
                                                    const lang::ast::AssignmentStatementPtr &stmt)
    {
        std::string result;
        std::visit(
            [&](auto &&ptr)
            {
                using T = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<T, lang::ast::LiteralPtr>)
                {
                    std::visit(
                        [&](auto &&value)
                        {
                            using M = std::decay_t<decltype(value)>;
                            if constexpr (std::is_same_v<M, std::string> or
                                          std::is_same_v<M, std::int64_t> or
                                          std::is_same_v<M, bool>)
                            {
                                result = fmt::format(withVariableFormat, value, stmt->name);
                            }
                            // TODO: Add set translator
                        },
                        ptr->value);
                }
            },
            *stmt->valueExpr);
        return result;
    }

    static std::string TranslateQuantifierStatement(Context &context,
                                                    const lang::ast::QuantifierStatementPtr &stmt)
    {
        // TODO: ExperssionPtr - implement base translator
        switch (stmt->type)
        {
        case lang::ast::QuantifierType::ALL:
            return TranslateSelectionStatement<lang::ast::QuantifierType::ALL>(context, stmt->body);
        case lang::ast::QuantifierType::ANY:
            return TranslateSelectionStatement<lang::ast::QuantifierType::ANY>(context, stmt->body);
        default:
            throw std::runtime_error("Неизвестный тип квантора");
        }
    }
};
}; // namespace cypher