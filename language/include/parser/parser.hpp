#pragma once

#include "identifiers.hpp"
#include "literals.hpp"
#include "statements.hpp"
#include <ast/ast.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>

namespace lang::grammar
{

struct Block
{
    static constexpr auto rule = dsl::list(dsl::p<BodyStatement>, dsl::sep(dsl::semicolon));

    static constexpr auto value = lexy::as_list<ast::BodyStatementList> >>
                                  lexy::callback<ast::BlockPtr>(
                                      [](auto &&stmts)
                                      { return std::make_unique<ast::Block>(std::move(stmts)); });
};

struct Description
{
    static constexpr auto rule = LEXY_LIT("description:") >>
                                 dsl::while_(dsl::ascii::space) >> dsl::p<String> >> LEXY_LIT(";");
    static constexpr auto value = lexy::as_string<std::string>;
};

struct Priority
{
    struct PriorityError
    {
        static constexpr auto rule = LEXY_LIT("Error");
        static constexpr auto value = lexy::constant(ast::Priority::ERROR);
    };

    struct PriorityInfo
    {
        static constexpr auto rule = LEXY_LIT("Info");
        static constexpr auto value = lexy::constant(ast::Priority::INFO);
    };

    struct PriorityWarn
    {
        static constexpr auto rule = LEXY_LIT("Warn");
        static constexpr auto value = lexy::constant(ast::Priority::WARN);
    };

    static constexpr auto rule = LEXY_LIT("priority:") >> dsl::while_(dsl::ascii::space) >>
                                 (dsl::p<PriorityError> | dsl::p<PriorityInfo> |
                                  dsl::p<PriorityWarn>) >>
                                 LEXY_LIT(";");
    static constexpr auto value = lexy::forward<ast::Priority>;
};

struct RuleDecl
{
    static constexpr auto whitespace = dsl::ascii::space | dsl::ascii::newline;
    static constexpr auto rule = LEXY_LIT("rule") >> dsl::p<Identifier> >>
                                 dsl::curly_bracketed(dsl::p<Description> + dsl::p<Priority> +
                                                      dsl::p<Block>);

    static constexpr auto value = lexy::callback<ast::Rule>(
        [](std::string &&name, std::string &&desc, ast::Priority prio,
           ast::BlockPtr &&block) -> ast::Rule
        { return ast::Rule(std::move(name), std::move(desc), prio, std::move(block)); });
};

auto Parse(const std::string& input)
{
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    auto result = lexy::parse<lang::grammar::RuleDecl>(strInput, lexy_ext::report_error);
    if(not result.has_value())
    {
        throw std::runtime_error{"Failed parsing program"};
    }
    return result;
}

} // namespace lang::grammar