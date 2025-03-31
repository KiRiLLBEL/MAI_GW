#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include <algorithm>
#include <ast/ast.hpp>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace lang::ast::json
{
template <typename T> class Serializer
{
public:
    nlohmann::json operator()(const T & /*value*/) const
    {
        return nlohmann::json{{"unimplemented", "No serialization for this type"}};
    }
};

template <typename... Ts> class Serializer<std::variant<Ts...>>
{
public:
    nlohmann::json operator()(const std::variant<Ts...> &var) const
    {
        return std::visit([&](auto &subValue) -> nlohmann::json
                          { return Serializer<std::decay_t<decltype(subValue)>>{}(subValue); },
                          var);
    }
};

template <typename T> class Serializer<std::unique_ptr<T>>
{
public:
    nlohmann::json operator()(const std::unique_ptr<T> &ptr) const
    {
        if (!ptr)
        {
            throw std::runtime_error{"broken AST: ptr is null"};
        }

        return Serializer<T>{}(*ptr);
    }
};

template <KeywordSets K> class Serializer<KeywordExpr<K>>
{
public:
    nlohmann::json operator()(const KeywordExpr<K> & /*unused*/) const
    {
        static constexpr auto type = "keyword";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut[type] = magic_enum::enum_name(K);
        return jOut;
    }
};

template <typename T> class Serializer<LiteralExpr<T>>
{
public:
    nlohmann::json operator()(const LiteralExpr<T> &lit) const
    {
        static constexpr auto type = "literal";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut[type] = lit.value;
        return jOut;
    }
};

template <> class Serializer<SetExpr>
{
public:
    nlohmann::json operator()(const SetExpr &expr) const
    {
        static constexpr auto type = "set";
        nlohmann::json jOut;
        const auto view = expr.items | std::views::transform(Serializer<ExpressionPtr>{});
        jOut["expression"] = type;
        jOut[type] = nlohmann::json::array();
        for (const auto &item : view)
        {
            jOut[type].push_back(item);
        }
        return jOut;
    }
};

template <> class Serializer<VariableExpr>
{
public:
    nlohmann::json operator()(const VariableExpr &var) const
    {
        static constexpr auto type = "variable";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut[type] = var.name;
        return jOut;
    }
};

template <ExprType K> class Serializer<AccessExpr<K>>
{
public:
    nlohmann::json operator()(const AccessExpr<K> &expr) const
    {
        nlohmann::json jOut;
        jOut["type"] = magic_enum::enum_name(K);
        jOut["operand"] = Serializer<ExpressionPtr>{}(expr.operand);
        jOut["property"] = expr.prop;
        return jOut;
    }
};

template <ExprType K> class Serializer<UnaryExpr<K>>
{
public:
    nlohmann::json operator()(const UnaryExpr<K> &expr) const
    {
        nlohmann::json jOut;
        jOut["type"] = magic_enum::enum_name(K);
        jOut["operand"] = Serializer<ExpressionPtr>{}(expr.operand);
        return jOut;
    }
};

template <> class Serializer<CallExpr>
{
public:
    nlohmann::json operator()(const CallExpr &expr) const
    {
        static constexpr auto type = "call";
        nlohmann::json jOut;
        const auto view = expr.args | std::views::transform(Serializer<ExpressionPtr>{});
        jOut["expression"] = type;
        jOut["name"] = expr.functionName;
        jOut["args"] = nlohmann::json::array();
        for (const auto &item : view)
        {
            jOut["args"].push_back(item);
        }
        return jOut;
    }
};

template <template <ExprType> class T, ExprType U>
concept BinaryExpression = requires(T<U> expr) {
    requires std::same_as<decltype(expr.left), ExpressionPtr>;
    requires std::same_as<decltype(expr.right), ExpressionPtr>;
};

template <template <ExprType> class T, ExprType U>
    requires BinaryExpression<T, U>
class Serializer<T<U>>
{
public:
    nlohmann::json operator()(const T<U> &expr) const
    {
        nlohmann::json jOut;
        jOut["type"] = magic_enum::enum_name(U);
        jOut["left"] = Serializer<ExpressionPtr>{}(expr.left);
        jOut["right"] = Serializer<ExpressionPtr>{}(expr.right);
        return jOut;
    }
};

template <> class Serializer<TernaryExpr>
{
public:
    nlohmann::json operator()(const TernaryExpr &expr) const
    {
        static constexpr auto type = "ternary";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["cond"] = Serializer<ExpressionPtr>{}(expr.condition);
        jOut["then"] = Serializer<ExpressionPtr>{}(expr.thenExpr);
        jOut["else"] = Serializer<ExpressionPtr>{}(expr.elseExpr);
        return jOut;
    }
};

template <> class Serializer<AssignmentStatement>
{
public:
    nlohmann::json operator()(const AssignmentStatement &stmt) const
    {
        static constexpr auto type = "assignment";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["name"] = stmt.name;
        jOut["expression"] = Serializer<ExpressionPtr>{}(stmt.valueExpr);
        return jOut;
    }
};

template <QuantifierType T> struct Serializer<QuantifierStatement<T>>
{
public:
    nlohmann::json operator()(const QuantifierStatement<T> &stmt) const
    {
        nlohmann::json jOut;
        jOut["type"] = magic_enum::enum_name(T);
        jOut["expression"] = Serializer<SelectionStatementPtr>{}(stmt.body);
        return jOut;
    }
};

template <> class Serializer<SelectionStatement>
{
public:
    nlohmann::json operator()(const SelectionStatement &stmt) const
    {
        static constexpr auto type = "statement";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["args"] = stmt.identifiersList;
        jOut["source"] = Serializer<ExpressionPtr>{}(stmt.source);
        jOut["predicate"] = Serializer<PredicatePtr>{}(stmt.predicate);
        return jOut;
    }
};

template <> class Serializer<IfThen>
{
public:
    nlohmann::json operator()(const IfThen &stmt) const
    {
        static constexpr auto type = "if-then";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["cond"] = Serializer<ExpressionPtr>{}(stmt.expr);
        jOut["then"] = Serializer<PredicatePtr>{}(stmt.then);
        return jOut;
    }
};

template <> class Serializer<IfThenElse>
{
public:
    nlohmann::json operator()(const IfThenElse &stmt) const
    {
        static constexpr auto type = "if-then-else";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["cond"] = Serializer<ExpressionPtr>{}(stmt.expr);
        jOut["then"] = Serializer<PredicatePtr>{}(stmt.then);
        jOut["else"] = Serializer<PredicatePtr>{}(stmt.els);
        return jOut;
    }
};

template <> class Serializer<StatementExpression>
{
public:
    nlohmann::json operator()(const StatementExpression &stmt) const
    {
        static constexpr auto type = "statement-expression";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["expression"] = Serializer<ExpressionPtr>{}(stmt.expr);
        return jOut;
    }
};

template <> class Serializer<FilteredStatement>
{
public:
    nlohmann::json operator()(const FilteredStatement &stmt) const
    {
        static constexpr auto type = "filter-statement";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["expression"] = Serializer<StatementExpressionPtr>{}(stmt.expr);
        jOut["quantifier"] = Serializer<QuantifierPtr>{}(stmt.quant);
        return jOut;
    }
};

template <> class Serializer<ExceptStatement>
{
public:
    nlohmann::json operator()(const ExceptStatement &stmt) const
    {
        static constexpr auto type = "except-statement";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["statement"] = Serializer<QuantifierPtr>{}(stmt.inner);
        return jOut;
    }
};

template <> class Serializer<Block>
{
public:
    nlohmann::json operator()(const Block &stmt) const
    {
        static constexpr auto type = "block";
        const auto view = stmt.statements | std::views::transform(Serializer<BodyStatementPtr>{});
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["statements"] = nlohmann::json::array();
        for (const auto &item : view)
        {
            jOut["statements"].push_back(item);
        }
        return jOut;
    }
};

template <> class Serializer<Rule>
{
public:
    nlohmann::json operator()(const Rule &stmt) const
    {
        static constexpr auto type = "rule";
        nlohmann::json jOut;
        jOut["type"] = type;
        jOut["name"] = stmt.name;
        jOut["description"] = stmt.description;
        jOut["priority"] = magic_enum::enum_name(stmt.priority);
        jOut["blocks"] = Serializer<BlockPtr>{}(stmt.calls);
        return jOut;
    }
};

template <typename U> std::string Serialize(U &&value)
{
    using CleanType = std::decay_t<U>;
    return nlohmann::to_string(Serializer<CleanType>{}(std::forward<U>(value)));
}

} // namespace lang::ast::json
