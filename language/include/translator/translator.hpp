#pragma once

#include "ast/statement.hpp"
#include <translator/constant.hpp>
#include <translator/context.hpp>
#include <translator/format.hpp>
#include <translator/helpers.hpp>

#include <ast/ast.hpp>
#include <ast/expression.hpp>

#include <fmt/args.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <magic_enum/magic_enum.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

namespace lang::ast::cypher
{

using TranslationResult = std::string;

namespace
{

class QuantifierGuard
{
public:
    explicit QuantifierGuard(TranslatorContext &ctx) : ctx_(ctx)
    {
        ++ctx.quantifierLevel;
    }

    ~QuantifierGuard()
    {
        --ctx_.quantifierLevel;
    }

private:
    TranslatorContext &ctx_;
};

class ExceptGuard
{
public:
    explicit ExceptGuard(TranslatorContext &ctx) : ctx_(ctx)
    {
        ctx.exceptRule = true;
    }

    ~ExceptGuard()
    {
        ctx_.exceptRule = false;
    }

private:
    TranslatorContext &ctx_;
};

class TranslatorBase
{
protected:
    TranslatorContext &ctx;

public:
    explicit TranslatorBase(TranslatorContext &context) : ctx(context)
    {
    }
    virtual ~TranslatorBase() = default;
};
} // namespace

template <typename T> class Translator : TranslatorBase
{
public:
    TranslationResult operator()(const T & /*unused*/) const
    {
        return "unimplemented translation";
    }
};

template <typename... Ts> class Translator<std::variant<Ts...>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const std::variant<Ts...> &var) const
    {
        return std::visit([&](auto &subValue) -> TranslationResult
                          { return Translator<std::decay_t<decltype(subValue)>>{ctx}(subValue); },
                          var);
    }
};

template <typename T> class Translator<std::unique_ptr<T>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const std::unique_ptr<T> &ptr) const
    {
        if (!ptr)
        {
            throw std::runtime_error{"broken AST: ptr is null in translator"};
        }
        return Translator<T>{ctx}(*ptr);
    }
};

template <KeywordSets K> class Translator<KeywordExpr<K>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const KeywordExpr<K> & /*unused*/) const
    {
        return KeywordMap(K);
    }
};

template <typename T> class Translator<LiteralExpr<T>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const LiteralExpr<T> &lit) const
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return fmt::format("\"{}\"", lit.value);
        }
        return fmt::to_string(lit.value);
    }
};

template <> class Translator<SetExpr> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const SetExpr &expr) const
    {
        return fmt::format(
            "[{}]",
            fmt::join(expr.items | std::views::transform(Translator<ExpressionPtr>{ctx}), ", "));
    }
};

template <> class Translator<VariableExpr> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const VariableExpr &var) const
    {
        if (not ctx.variableTable.contains(var.name))
        {
            throw ErrorHelper(variableError, var.name);
        }
        return var.name;
    }
};

template <ExprType K> class Translator<AccessExpr<K>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const AccessExpr<K> &expr) const
    {
        return fmt::format(fmt::runtime(OperatorMap(K)),
                           Translator<ExpressionPtr>{ctx}(expr.operand), expr.prop);
    }
};

template <ExprType K> class Translator<UnaryExpr<K>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const UnaryExpr<K> &expr) const
    {
        return fmt::format(fmt::runtime(OperatorMap(K)),
                           Translator<ExpressionPtr>{ctx}(expr.operand));
    }
};

template <> class Translator<CallExpr> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const CallExpr &expr) const
    {
        if (not functionMap.contains(expr.functionName))
        {
            throw ErrorHelper(functionError, expr.functionName);
        }
        fmt::dynamic_format_arg_store<fmt::format_context> store;
        for (const auto &arg : expr.args | std::views::transform(Translator<ExpressionPtr>{ctx}))
        {
            store.push_back(arg);
        }
        return fmt::vformat(functionMap.at(expr.functionName), store);
    }
};

template <template <ExprType> class T, ExprType U>
concept BinaryExpression = requires(T<U> tmp) {
    requires std::same_as<decltype(tmp.left), ExpressionPtr>;
    requires std::same_as<decltype(tmp.right), ExpressionPtr>;
};

template <template <ExprType> class T, ExprType U>
    requires BinaryExpression<T, U>
class Translator<T<U>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const T<U> &expr) const
    {
        const auto left = Translator<ExpressionPtr>{ctx}(expr.left);
        const auto right = Translator<ExpressionPtr>{ctx}(expr.right);
        return fmt::format(fmt::runtime(OperatorMap(U)), left, right);
    }
};

template <> class Translator<TernaryExpr> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const TernaryExpr &expr) const
    {
        const auto cond = Translator<ExpressionPtr>{ctx}(expr.condition);
        const auto then = Translator<ExpressionPtr>{ctx}(expr.thenExpr);
        const auto els = Translator<ExpressionPtr>{ctx}(expr.elseExpr);
        return fmt::format(ternaryFormat, cond, then, els);
    }
};

template <> class Translator<AssignmentStatement> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const AssignmentStatement &stmt) const
    {
        ctx.variableTable.insert(stmt.name);
        ctx.variableType[stmt.name] = KeywordSets::NONE;
        const auto value = Translator<ExpressionPtr>{ctx}(stmt.valueExpr);
        return fmt::format(withAssignmentFormat, value, stmt.name);
    }
};

template <typename T>
concept BasicSource = std::is_same_v<T, SystemPtr> || std::is_same_v<T, ContainerPtr> ||
                      std::is_same_v<T, ComponentPtr> || std::is_same_v<T, CodePtr> ||
                      std::is_same_v<T, DeployPtr> || std::is_same_v<T, InfrastructurePtr>;

template <typename T> struct SourceHandler
{
    TranslationResult operator()(const std::vector<std::string> & /*unused*/, const T & /*unused*/,
                                 TranslatorContext & /*unused*/) const
    {
        throw std::runtime_error{"BUG"};
    };
};

template <typename... Ts> struct SourceHandler<std::variant<Ts...>>
{
    TranslationResult operator()(const std::vector<std::string> &args,
                                 const std::variant<Ts...> &var, TranslatorContext &ctx) const
    {
        return std::visit(
            [&](auto &subValue) -> TranslationResult
            { return SourceHandler<std::decay_t<decltype(subValue)>>{}(args, subValue, ctx); },
            var);
    }
};

template <> struct SourceHandler<ExpressionPtr>
{
    TranslationResult operator()(const std::vector<std::string> &args, const ExpressionPtr &elem,
                                 TranslatorContext &ctx)
    {
        return SourceHandler<Expression>{}(args, *elem, ctx);
    }
};

template <> struct SourceHandler<CallPtr>
{
    TranslationResult operator()(const std::vector<std::string> &args, const CallPtr &elem,
                                 TranslatorContext &ctx)
    {
        if (elem->functionName == "route")
        {
            std::string result = fmt::format("MATCH p = {}", Translator<CallPtr>{ctx}(elem));
            for (const auto &arg : args)
            {
                result += fmt::format(" UNWIND nodes(p) AS {}", arg);
                result += fmt::format(" WITH {}", arg);
                ctx.variableTable.insert(arg);
            }
            result += " WHERE";
            for (size_t i = 0; i < args.size(); ++i)
            {
                for (size_t j = i + 1; j < args.size(); ++j)
                {
                    result += fmt::format(" {} <> {} AND", args[i], args[j]);
                }
            }

            return result;
        }

        if (elem->functionName == "instance")
        {
            if (args.empty())
            {
                throw std::runtime_error{"Empty selector list"};
            }
            std::string result = fmt::format("MATCH ({}:ContainerInstance)-{}", args.front(),
                                             Translator<CallPtr>{ctx}(elem));
            result += " WHERE";
            ctx.variableTable.insert(args.front());
            return result;
        }

        throw std::runtime_error{"Unsupported function"};
    }
};

template <BasicSource T> struct SourceHandler<T>
{
    TranslationResult operator()(const std::vector<std::string> &args, const T &elem,
                                 TranslatorContext &ctx)
    {
        std::string result = "MATCH";
        bool first = true;
        for (const auto &arg : args)
        {
            if (!first)
            {
                result += ", ";
            }
            first = false;
            result += fmt::format(" ({}:{})", arg, Translator<T>{ctx}(elem));
            ctx.variableTable.insert(arg);
            ctx.variableType[arg] = T::element_type::kind;
        }
        result += " WHERE";
        for (size_t i = 0; i < args.size(); ++i)
        {
            for (size_t j = i + 1; j < args.size(); ++j)
            {
                result += fmt::format(" {} <> {} AND", args[i], args[j]);
            }
        }
        return result;
    }
};

template <> struct SourceHandler<VariablePtr>
{
    TranslationResult operator()(const std::vector<std::string> &args, const VariablePtr &elem,
                                 TranslatorContext &ctx)
    {
        std::string result = "MATCH";
        if (ctx.variableType[elem->name] == KeywordSets::DEPLOY)
        {
            for (const auto &arg : args)
            {
                result += fmt::format(
                    " ({})-[:CONTAINS*]->(:ContainerInstance)-[:INSTANCE_OF]->({}:Container)",
                    Translator<VariablePtr>{ctx}(elem), arg);
                ctx.variableTable.insert(arg);
                ctx.variableType[arg] = KeywordSets::CONTAINER;
            }
        }
        else
        {
            for (const auto &arg : args)
            {
                result += fmt::format(" ({})-[:CONTAINS*]->({})",
                                      Translator<VariablePtr>{ctx}(elem), arg);
                ctx.variableTable.insert(arg);
                ctx.variableType[arg] = SetsMapping(ctx.variableType[elem->name]);
            }
        }
        result += " WHERE";
        for (size_t i = 0; i < args.size(); ++i)
        {
            for (size_t j = i + 1; j < args.size(); ++j)
            {
                result += fmt::format(" {} <> {} AND", args[i], args[j]);
            }
        }
        return result;
    }
};

template <typename P>
concept FilteredPredicate = std::is_same_v<std::decay_t<P>, FilteredStatementPtr>;

template <QuantifierType Q> class Translator<QuantifierStatement<Q>> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const QuantifierStatement<Q> &stmt) const
    {
        QuantifierGuard guard{ctx};

        return std::visit(
            [&](auto &&pred) -> TranslationResult
            {
                using PredT = std::decay_t<decltype(pred)>;
                using T = std::decay_t<decltype(stmt.source)>;
                if constexpr (FilteredPredicate<PredT>)
                {
                    if (ctx.quantifierLevel == 1 and ctx.exceptRule)
                    {
                        const auto predicate = Translator<StatementExpressionPtr>{ctx}(pred->expr);
                        const auto quant = Translator<QuantifierPtr>{ctx}(pred->quant);
                        return fmt::format(fmt::runtime(QuantifierExceptMap(Q)),
                                           predicate + " AND ", quant);
                    }
                    if (ctx.quantifierLevel == 1)
                    {
                        ctx.returns = stmt.identifiersList;
                        const auto source =
                            SourceHandler<T>{}(stmt.identifiersList, stmt.source, ctx);
                        const auto expr = Translator<StatementExpressionPtr>{ctx}(pred->expr);
                        const auto quant = Translator<QuantifierPtr>{ctx}(pred->quant);
                        return fmt::format(fmt::runtime(QuantifierStartMap(Q)), source,
                                           expr + " AND ", quant);
                    }
                    const auto source = SourceHandler<T>{}(stmt.identifiersList, stmt.source, ctx);
                    const auto expr = Translator<StatementExpressionPtr>{ctx}(pred->expr);
                    const auto quant = Translator<QuantifierPtr>{ctx}(pred->quant);
                    return fmt::format(fmt::runtime(QuantifierMap(Q)), source, expr + " AND ",
                                       quant);
                }
                if (ctx.quantifierLevel == 1 and ctx.exceptRule)
                {
                    return fmt::format(fmt::runtime(QuantifierExceptMap(Q)), "",
                                       Translator<PredicatePtr>{ctx}(stmt.predicate));
                }
                if (ctx.quantifierLevel == 1)
                {
                    const auto source = SourceHandler<T>{}(stmt.identifiersList, stmt.source, ctx);
                    const auto predicate = Translator<PredicatePtr>{ctx}(stmt.predicate);
                    ctx.returns = stmt.identifiersList;
                    return fmt::format(fmt::runtime(QuantifierStartMap(Q)), source, "", predicate);
                }
                const auto source = SourceHandler<T>{}(stmt.identifiersList, stmt.source, ctx);
                const auto predicate = Translator<PredicatePtr>{ctx}(stmt.predicate);
                return fmt::format(fmt::runtime(QuantifierMap(Q)), source, "", predicate);
            },
            *stmt.predicate);
    }
};

template <> class Translator<IfThen> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const IfThen &stmt) const
    {
        const auto expr = Translator<ExpressionPtr>{ctx}(stmt.expr);
        const auto then = Translator<PredicatePtr>{ctx}(stmt.then);
        return fmt::format(ifThenFormat, expr, then);
    }
};

template <> class Translator<IfThenElse> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const IfThenElse &stmt) const
    {
        const auto expr = Translator<ExpressionPtr>{ctx}(stmt.expr);
        const auto then = Translator<PredicatePtr>{ctx}(stmt.then);
        const auto els = Translator<PredicatePtr>{ctx}(stmt.els);
        return fmt::format(ifThenElseFormat, expr, then, els);
    }
};

template <> class Translator<StatementExpression> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const StatementExpression &stmt) const
    {
        return Translator<ExpressionPtr>{ctx}(stmt.expr);
    }
};

template <> class Translator<FilteredStatement> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const FilteredStatement &stmt) const
    {
        const auto expr = Translator<StatementExpressionPtr>{ctx}(stmt.expr);
        const auto quant = Translator<QuantifierPtr>{ctx}(stmt.quant);
        return fmt::format("{} AND {}", expr, quant);
    }
};

template <> class Translator<ExceptStatement> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const ExceptStatement &stmt) const
    {
        ExceptGuard guard{ctx};
        return fmt::format("AND NOT ( {} )", Translator<QuantifierPtr>{ctx}(stmt.inner));
    }
};

template <> class Translator<Block> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const Block &stmt) const
    {
        return fmt::to_string(fmt::join(
            stmt.statements | std::views::transform(Translator<BodyStatementPtr>{ctx}), " "));
    }
};

template <> class Translator<Rule> : TranslatorBase
{
public:
    using TranslatorBase::TranslatorBase;
    TranslationResult operator()(const Rule &stmt) const
    {
        std::string result = fmt::format(ruleNameFormat, stmt.name);
        result += "\n" + fmt::format(descriptionFormat, stmt.description);
        result += "\n" + fmt::format(priorityFormat, magic_enum::enum_name(stmt.priority)) + "\n";
        return result + Translator<BlockPtr>{ctx}(stmt.calls) +
               fmt::format(" RETURN {}", fmt::join(ctx.returns, " ,"));
    }
};

template <typename U> TranslationResult Translate(U &&value)
{
    using CleanType = std::decay_t<U>;
    TranslatorContext context;
    Translator<CleanType> translator{context};
    return translator(std::forward<U>(value));
}

}; // namespace lang::ast::cypher