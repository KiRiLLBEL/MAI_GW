#pragma once

#include "ast/expression.hpp"
#include <lexy/callback/string.hpp>
#include <lexy/dsl/capture.hpp>
#include <lexy/dsl/expression.hpp>
#include <parser/identifiers.hpp>
#include <parser/literals.hpp>

#include <string>
#include <utility>

namespace lang::grammar
{
namespace dsl = lexy::dsl;

namespace
{

template <typename T, typename Lhs, typename Op, typename Rhs> auto constexpr CreateCallback()
{
    return [](Lhs lhs, Op, Rhs rhs) -> ast::ExpressionPtr
    {
        return std::make_unique<ast::Expression>(std::make_unique<typename T::element_type>(
            std::forward<Lhs>(lhs), std::forward<Rhs>(rhs)));
    };
}

template <typename T, typename Op, typename Expr> auto constexpr CreateCallback()
{
    return [](Op, Expr expr) -> ast::ExpressionPtr
    {
        return std::make_unique<ast::Expression>(
            std::make_unique<typename T::element_type>(std::forward<Expr>(expr)));
    };
}

template <typename T, typename Op> auto constexpr CreateCallbackBinary()
{
    return CreateCallback<T, ast::ExpressionPtr, Op, ast::ExpressionPtr>();
}

} // namespace

struct NestedExpr : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct ExpressionProduct>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct FuncArgs : lexy::token_production
{
    static constexpr auto rule = dsl::parenthesized.opt_list(
        (dsl::p<NestedExpr>), dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));
    static constexpr auto value = lexy::as_list<std::vector<ast::ExpressionPtr>>;
};

struct IdentifierExpr
{
    static constexpr auto rule = dsl::capture(dsl::p<Identifier>) >> dsl::opt(dsl::p<FuncArgs>);
    static constexpr auto value = lexy::bind(
        lexy::callback<ast::ExpressionPtr>(
            [](const auto &capture, auto &&lex, std::string &&name,
               lexy::nullopt &&) -> ast::ExpressionPtr
            {
                return std::make_unique<ast::Expression>(
                    std::make_unique<ast::VariableExpr>(std::move(name), std::move(capture(lex))));
            },
            [](const auto &capture, auto &&lex, std::string &&name,
               auto &&lst) -> ast::ExpressionPtr
            {
                return std::make_unique<ast::Expression>(std::make_unique<ast::CallExpr>(
                    std::move(name), std::move(lst), std::move(capture(lex))));
            }),
        lexy::parse_state, lexy::values);
};

struct FactorExpr
{
    static constexpr auto rule = dsl::parenthesized(dsl::p<NestedExpr>) | dsl::p<Keyword> |
                                 dsl::p<Literal> | dsl::p<IdentifierExpr>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct ExpressionProduct : lexy::expression_production
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto atom = dsl::p<FactorExpr>;

    static constexpr auto opPlus = dsl::op(dsl::lit_c<'+'>);
    static constexpr auto opMinus = dsl::op(dsl::lit_c<'-'>);
    static constexpr auto opMult = dsl::op(dsl::lit_c<'*'>);
    static constexpr auto opDiv = dsl::op(dsl::lit_c<'/'>);
    static constexpr auto opNeg = dsl::op(LEXY_LIT("not"));
    static constexpr auto opEqual = dsl::op(LEXY_LIT("=="));
    static constexpr auto opNotEqual = dsl::op(LEXY_LIT("/="));
    static constexpr auto opLess = dsl::op(dsl::lit_c<'<'>);
    static constexpr auto opGreater = dsl::op(dsl::lit_c<'>'>);
    static constexpr auto opLessEq = dsl::op(LEXY_LIT("<="));
    static constexpr auto opGreaterEq = dsl::op(LEXY_LIT(">="));
    static constexpr auto opIn = dsl::op(LEXY_LIT("in"));
    static constexpr auto opNotIn = dsl::op(LEXY_LIT("not in"));
    static constexpr auto opAnd = dsl::op(LEXY_LIT("and"));
    static constexpr auto opOr = dsl::op(LEXY_LIT("or"));
    static constexpr auto opXor = dsl::op(LEXY_LIT("xor"));
    static constexpr auto opAccess = dsl::op(dsl::lit_c<'.'>);
    static constexpr auto opSAccess = dsl::op(LEXY_LIT(".!"));

    struct AccessExpr : dsl::postfix_op
    {
        static constexpr auto op = dsl::op<opAccess>(LEXY_LIT(".") >> dsl::p<Identifier>) /
                                   dsl::op<opSAccess>(LEXY_LIT(".!") >> dsl::p<Identifier>);
        using operand = dsl::atom;
    };

    struct UnaryExpr : dsl::prefix_op
    {
        static constexpr auto op = opNeg;
        using operand = AccessExpr;
    };

    struct MultExpr : dsl::infix_op_left
    {
        static constexpr auto op = opMult / opDiv;
        using operand = UnaryExpr;
    };

    struct AddExpr : dsl::infix_op_left
    {
        static constexpr auto op = opPlus / opMinus;
        using operand = MultExpr;
    };

    struct CompareExpr : dsl::infix_op_left
    {
        static constexpr auto op =
            opEqual / opNotEqual / opGreater / opLess / opGreaterEq / opLessEq / opIn / opNotIn;
        using operand = AddExpr;
    };

    struct LogicalExpr : dsl::infix_op_left
    {
        static constexpr auto op = opAnd / opXor / opOr;
        using operand = CompareExpr;
    };

    struct TernaryExpr : dsl::infix_op_single
    {
        static constexpr auto op =
            dsl::op<void>(LEXY_LIT("?") >> (dsl::p<NestedExpr> + dsl::lit_c<':'>));
        using operand = LogicalExpr;
    };

    using operation = TernaryExpr;

    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](ast::ExpressionPtr &&expr) { return expr; },
        CreateCallback<ast::NegationPtr, lexy::op<opNeg>, ast::ExpressionPtr>(),
        CreateCallbackBinary<ast::MultiplyPtr, lexy::op<opMult>>(),
        CreateCallbackBinary<ast::DivisionPtr, lexy::op<opDiv>>(),
        CreateCallbackBinary<ast::AddPtr, lexy::op<opPlus>>(),
        CreateCallbackBinary<ast::MinusPtr, lexy::op<opMinus>>(),
        CreateCallbackBinary<ast::NotEqualPtr, lexy::op<opNotEqual>>(),
        CreateCallbackBinary<ast::EqualPtr, lexy::op<opEqual>>(),
        CreateCallbackBinary<ast::GreaterPtr, lexy::op<opGreater>>(),
        CreateCallbackBinary<ast::LessPtr, lexy::op<opLess>>(),
        CreateCallbackBinary<ast::GreateEqualPtr, lexy::op<opGreaterEq>>(),
        CreateCallbackBinary<ast::LessEqualPtr, lexy::op<opLessEq>>(),
        CreateCallbackBinary<ast::InPtr, lexy::op<opIn>>(),
        CreateCallbackBinary<ast::NotInPtr, lexy::op<opNotIn>>(),
        CreateCallbackBinary<ast::AndPtr, lexy::op<opAnd>>(),
        CreateCallbackBinary<ast::XorPtr, lexy::op<opXor>>(),
        CreateCallbackBinary<ast::OrPtr, lexy::op<opOr>>(),
        [](ast::ExpressionPtr &&ifExpr, ast::ExpressionPtr &&thenExpr,
           ast::ExpressionPtr &&elseExpr)
        {
            return std::make_unique<ast::Expression>(std::make_unique<ast::TernaryExpr>(
                std::move(ifExpr), std::move(thenExpr), std::move(elseExpr)));
        },
        CreateCallback<ast::AccessExprPtr, ast::ExpressionPtr, decltype(opAccess), std::string>(),
        CreateCallback<ast::SafeAccessExprPtr, ast::ExpressionPtr, decltype(opSAccess),
                       std::string>());
};

} // namespace lang::grammar