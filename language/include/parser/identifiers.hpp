#pragma once
#include "ast/expression.hpp"
#include "parser/literals.hpp"
#include <array>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/object.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/dsl.hpp>

#include <lexy/dsl/identifier.hpp>
#include <lexy/dsl/literal.hpp>
#include <memory>
#include <string>

namespace lang::grammar
{

namespace dsl = lexy::dsl;

struct Identifier : lexy::token_production
{
    static constexpr auto rule = []
    {
        constexpr auto id =
            dsl::identifier(dsl::ascii::alpha_digit_underscore, dsl::ascii::alpha_digit_underscore);
        return id.reserve(
            LEXY_KEYWORD("not", id), LEXY_KEYWORD("in", id), LEXY_KEYWORD("or", id),
            LEXY_KEYWORD("and", id), LEXY_KEYWORD("xor", id), LEXY_KEYWORD("all", id),
            LEXY_KEYWORD("exist", id), LEXY_KEYWORD("true", id), LEXY_KEYWORD("false", id),
            LEXY_KEYWORD("if", id), LEXY_KEYWORD("then", id), LEXY_KEYWORD("else", id),
            LEXY_KEYWORD("none", id), LEXY_KEYWORD("except", id), LEXY_KEYWORD("priority", id),
            LEXY_KEYWORD("description", id), LEXY_KEYWORD("system", id),
            LEXY_KEYWORD("container", id), LEXY_KEYWORD("component", id), LEXY_KEYWORD("code", id),
            LEXY_KEYWORD("deploy", id), LEXY_KEYWORD("infrastructure", id));
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

struct Variable : lexy::token_production
{
    static constexpr auto rule = dsl::p<Identifier>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string &&varName) -> ast::ExpressionPtr
        {
            auto var = std::make_unique<ast::VariableExpr>(std::move(varName));
            return std::make_unique<ast::Expression>(std::move(var));
        });
};

struct FuncArgs : lexy::token_production
{
    static constexpr auto rule = dsl::curly_bracketed.opt_list(
        (dsl::p<Literal> | dsl::p<Variable>) >> dsl::while_(dsl::ascii::space),
        dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));
    static constexpr auto value = lexy::as_list<std::vector<ast::ExpressionPtr>>;
};

struct FunctionCall
{
    static constexpr auto rule = dsl::p<Identifier> >> dsl::p<FuncArgs>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string name, std::vector<ast::ExpressionPtr> args)
        {
            return std::make_unique<ast::Expression>(
                std::make_unique<ast::CallExpr>(std::move(name), std::move(args)));
        });
};

struct Keyword
{
    struct System
    {
        static constexpr auto rule = LEXY_LIT("system");
        static constexpr auto value = lexy::callback<ast::SystemPtr>(
            []() -> ast::SystemPtr { return std::make_unique<ast::SystemPtr::element_type>(); });
    };

    struct Container
    {
        static constexpr auto rule = LEXY_LIT("container");
        static constexpr auto value = lexy::callback<ast::ContainerPtr>(
            []() -> ast::ContainerPtr
            { return std::make_unique<ast::ContainerPtr::element_type>(); });
    };

    struct Component
    {
        static constexpr auto rule = LEXY_LIT("component");
        static constexpr auto value = lexy::callback<ast::ComponentPtr>(
            []() -> ast::ComponentPtr
            { return std::make_unique<ast::ComponentPtr::element_type>(); });
    };

    struct Code
    {
        static constexpr auto rule = LEXY_LIT("code");
        static constexpr auto value = lexy::callback<ast::CodePtr>(
            []() -> ast::CodePtr { return std::make_unique<ast::CodePtr::element_type>(); });
    };

    struct Infrastructure
    {
        static constexpr auto rule = LEXY_LIT("infrastructure");
        static constexpr auto value = lexy::callback<ast::InfrastructurePtr>(
            []() -> ast::InfrastructurePtr
            { return std::make_unique<ast::InfrastructurePtr::element_type>(); });
    };

    struct Deploy
    {
        static constexpr auto rule = LEXY_LIT("deploy");
        static constexpr auto value = lexy::callback<ast::DeployPtr>(
            []() -> ast::DeployPtr { return std::make_unique<ast::DeployPtr::element_type>(); });
    };

    static constexpr auto rule = dsl::p<System> | dsl::p<System> | dsl::p<Container> |
                                 dsl::p<Component> | dsl::p<Code> | dsl::p<Infrastructure> |
                                 dsl::p<Deploy>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](auto &&item) { return std::make_unique<ast::Expression>(std::move(item)); });
};

} // namespace lang::grammar