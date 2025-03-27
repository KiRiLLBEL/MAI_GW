#pragma once

#include "identifiers.hpp"
#include "literals.hpp"
#include "statements.hpp"
#include <ast/ast.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

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

// struct program
// {
//     static constexpr auto rule = dsl::list(dsl::p<rule_decl>);
//     static constexpr auto value = lexy::as_list<std::vector<ast::Rule>>;
// };

// inline std::optional<std::vector<ast::Rule>> parse_dsl(const std::string& input)
// {
//     auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
//     auto result = lexy::parse<program>(str_input, lexy_ext::report_error);

//     if (result.has_value())
//         return result.value();
//     else
//         return std::nullopt;
// }

} // namespace lang::grammar