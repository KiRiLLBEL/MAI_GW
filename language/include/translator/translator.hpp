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
    static std::string RuleTranslate(const lang::ast::Rule &rule)
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

    static std::string BlockIter(Context &context, const lang::ast::Block &block)
    {
        std::ostringstream oss;

        for (const auto &stmtPtr : block.statements)
        {
            if (!stmtPtr)
            {
                continue;
            }
            oss << StmtVisit(context, *stmtPtr);
            oss << "\n";
        }

        return oss.str();
    }

    static std::string StmtVisit(Context &context, const lang::ast::Statement &statement)
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
                    return AssignStmt(context, argPtr);
                }
                else if constexpr (std::is_same_v<T, lang::ast::QuantifierStatementPtr>)
                {
                    return QuantStmt(context, argPtr);
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
    static constexpr auto ExprInVisitor(auto &&arg) -> std::string
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
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), ExprVisit(arg->operand),
                               arg->prop);
        }
        else if constexpr (std::is_same_v<T, lang::ast::UnaryExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), ExprVisit(arg->operand));
        }
        else if constexpr (std::is_same_v<T, lang::ast::BinaryExprPtr>)
        {
            return fmt::format(fmt::runtime(OperatorMap(arg->op)), ExprVisit(arg->left),
                               ExprVisit(arg->right));
        }
        else if constexpr (std::is_same_v<T, lang::ast::TernaryExprPtr>)
        {
            return fmt::to_string(inDevelopmentFormat);
        }
        else if constexpr (std::is_same_v<T, lang::ast::CallPtr>)
        {
            return fmt::to_string(inDevelopmentFormat);
        }
        return "";
    };
    static std::string ExprVisit(const lang::ast::ExpressionPtr &expr)
    {
        return std::visit([&](auto &&arg) { return ExprInVisitor(arg); }, *expr);
    }

    template <lang::ast::QuantifierType quant>
    static std::string SelectionStmt(Context &context, const lang::ast::SelectionStatementPtr &stmt)
    {
        std::string info{};
        const auto selectionFrom = std::visit(
            [&](auto &&arg) -> std::string
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, lang::ast::VariablePtr>)
                {
                    if (not LiteralInSuperSet(arg) and not LiteralInContext(context, arg))
                    {
                        throw std::runtime_error(fmt::format(superSetsContainsError, arg->name));
                    }
                    if (LiteralInSuperSet(arg))
                    {
                        info = fmt::format(matchFormat, arg->name) + "\n" +
                               fmt::format(withCollectFormat, arg->name) + "\n";
                    }
                    return arg->name;
                }
                if constexpr (std::is_same_v<T, lang::ast::CallPtr>)
                {
                    if (not FuncCallEqualName(arg, routeFunc))
                    {
                        throw std::runtime_error(fmt::format(selectionFromRouteError));
                    }

                    info = fmt::format(quant == lang::ast::QuantifierType::ALL ? routeAllFormat
                                                                               : routeExistFormat,
                                       ExprVisit(arg->args.at(0)), ExprVisit(arg->args.at(1)));
                    return "route__";
                }
                throw std::runtime_error{"No valid AST tree"};
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
                                               names[index], selectionFrom, ExprVisit(arg));
                        }
                        if constexpr (std::is_same_v<T, lang::ast::StatementPtr>)
                        {
                            return fmt::format(callFormat, CreateMatchForCall(names),
                                               StmtVisit(context, *arg));
                        }
                        return fmt::to_string(inDevelopmentFormat);
                    },
                    stmt->body);
            }

            return fmt::format(quantFormat, magic_enum::enum_name(quant), names[index],
                               selectionFrom, self(self, names, index + 1));
        };
        return info + fmt::format(whereFormat, buildSelection(buildSelection, stmt->varNames, 0));
    }

    static std::string AssignStmt(Context & /*unused*/,
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

    static std::string QuantStmt(Context &context, const lang::ast::QuantifierStatementPtr &stmt)
    {
        switch (stmt->type)
        {
        case lang::ast::QuantifierType::ALL:
            return SelectionStmt<lang::ast::QuantifierType::ALL>(context, stmt->body);
        case lang::ast::QuantifierType::ANY:
            return SelectionStmt<lang::ast::QuantifierType::ANY>(context, stmt->body);
        default:
            throw std::runtime_error("Неизвестный тип квантора");
        }
    }
};
}; // namespace cypher