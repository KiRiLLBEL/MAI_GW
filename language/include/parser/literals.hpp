#pragma once

#include "ast/expression.hpp"

#include <lexy/callback.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/forward.hpp>
#include <lexy/callback/object.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/dsl.hpp>

#include <memory>
#include <string>
#include <type_traits>

namespace lang::grammar
{
namespace dsl = lexy::dsl;

struct Ws : lexy::token_production
{
    static constexpr auto rule = dsl::whitespace(dsl::ascii::space);
    static constexpr auto value = lexy::noop;
};

struct String : lexy::token_production
{
    static constexpr auto rule = dsl::quoted(dsl::code_point);
    static constexpr auto value = lexy::as_string<std::string>;
};

struct Boolean : lexy::token_production
{
    struct TrueImpl : lexy::transparent_production
    {
        static constexpr auto rule = LEXY_LIT("true");
        static constexpr auto value = lexy::constant(true);
    };

    struct FalseImpl : lexy::transparent_production
    {
        static constexpr auto rule = LEXY_LIT("false");
        static constexpr auto value = lexy::constant(false);
    };

    static constexpr auto rule = dsl::p<TrueImpl> | dsl::p<FalseImpl>;
    static constexpr auto value = lexy::callback<bool>([](bool val) -> bool { return val; });
};

struct Number : lexy::token_production
{
    static constexpr auto rule = dsl::integer<int64_t>;
    static constexpr auto value = lexy::forward<int64_t>;
};

struct LiteralSimple : lexy::token_production
{
    static constexpr auto rule = dsl::p<String> | dsl::p<Boolean> | dsl::p<Number>;

    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](auto &&item) -> ast::ExpressionPtr
        {
            return std::make_unique<ast::Expression>(
                std::make_unique<
                    ast::LiteralExpr<typename std::remove_reference_t<decltype(item)>>>(
                    std::move(item)));
        });
};

struct Set : lexy::token_production
{
    static constexpr auto rule =
        dsl::square_bracketed.list(dsl::p<LiteralSimple> >> dsl::while_(dsl::ascii::space),
                                   dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));
    static constexpr auto
        value = lexy::as_list<std::vector<ast::ExpressionPtr>> >>
                lexy::callback<ast::ExpressionPtr>(
                    [](std::vector<ast::ExpressionPtr> &&setItems) -> ast::ExpressionPtr
                    {
                        auto lit = std::make_unique<ast::SetExpr>(std::move(setItems));
                        return std::make_unique<ast::Expression>(std::move(lit));
                    });
};

struct Literal : lexy::token_production
{
    static constexpr auto rule = dsl::p<Set> | dsl::p<LiteralSimple>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

} // namespace lang::grammar