#pragma once

#include <algorithm>
#include <ast/expression.hpp>
#include <ast/statement.hpp>
#include <lexy/callback.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/object.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/branch.hpp>
#include <lexy/grammar.hpp>
#include <memory>
#include <parser/expressions.hpp>
#include <parser/identifiers.hpp>

namespace lang::grammar
{

namespace dsl = lexy::dsl;
struct NestedQuantifier : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct Quantifier>;
    static constexpr auto value = lexy::forward<ast::QuantifierPtr>;
};

struct NestedPredicate : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct Predicate>;
    static constexpr auto value = lexy::forward<ast::PredicatePtr>;
};

struct NestedBaseStatement : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct BaseStatement>;
    static constexpr auto value = lexy::forward<ast::BaseStatementPtr>;
};

struct StmtExpression
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::p<ExpressionProduct>;
    static constexpr auto value = lexy::callback<ast::StatementExpressionPtr>(
        [](auto &&item) { return std::make_unique<ast::StatementExpression>(std::move(item)); });
};

struct FilteredStmt
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::p<StmtExpression> + LEXY_LIT(":") + dsl::p<NestedQuantifier>;
    static constexpr auto value = lexy::callback<ast::FilteredStatementPtr>(
        [](auto &&lhs, auto &&rhs)
        { return std::make_unique<ast::FilteredStatement>(std::move(lhs), std::move(rhs)); });
};

struct Predicate
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule =
        (dsl::lookahead(dsl::lit_c<':'>, dsl::lit_c<'{'>) >> dsl::p<FilteredStmt>) |
        (dsl::lookahead(dsl::lit_c<'{'>, dsl::lit_c<'}'>) >> dsl::p<NestedBaseStatement>) |
        dsl::else_ >> dsl::p<StmtExpression>;
    static constexpr auto value = lexy::callback<ast::PredicatePtr>(
        [](auto &&item) { return std::make_unique<ast::Predicate>(std::move(item)); });
};

struct Quantifier : lexy::token_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;

    struct SelectionList
    {
        static constexpr auto rule =
            dsl::list(dsl::p<Identifier> >> dsl::while_(dsl::ascii::space),
                      dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));

        static constexpr auto value = lexy::as_list<std::vector<std::string>>;
    };

    struct Source
    {
        static constexpr auto rule = dsl::p<IdentifierExpr> | dsl::p<Keyword>;
        static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
    };

    struct QuantifierTokenAll
    {
        static constexpr auto rule = LEXY_LIT("all") >>
                                     dsl::curly_bracketed(dsl::p<SelectionList> + LEXY_LIT("in") +
                                                          dsl::p<Source> + LEXY_LIT(":") +
                                                          dsl::p<Predicate>);
        static constexpr auto value = lexy::callback<ast::QuantifierPtr>(
            [](auto &&list, auto &&source, auto &&pred)
            {
                return std::make_unique<ast::QuantifierStatement<ast::QuantifierType::ALL>>(
                    std::move(list), std::move(source), std::move(pred));
            });
    };

    struct QuantifierTokenExist
    {
        static constexpr auto rule = LEXY_LIT("exist") >>
                                     dsl::curly_bracketed(dsl::p<SelectionList> + LEXY_LIT("in") +
                                                          dsl::p<Source> + LEXY_LIT(":") +
                                                          dsl::p<Predicate>);
        static constexpr auto value = lexy::callback<ast::QuantifierPtr>(
            [](auto &&list, auto &&source, auto &&pred)
            {
                return std::make_unique<ast::QuantifierStatement<ast::QuantifierType::ANY>>(
                    std::move(list), std::move(source), std::move(pred));
            });
    };
    static constexpr auto rule = dsl::p<QuantifierTokenAll> | dsl::p<QuantifierTokenExist>;
    static constexpr auto value = lexy::forward<ast::QuantifierPtr>;
};

struct Conditional : lexy::token_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    struct Full
    {
        static constexpr auto
            rule = LEXY_LIT("if") >> dsl::p<ExpressionProduct> >>
                   LEXY_LIT("then") >> dsl::p<Predicate> >> dsl::opt(LEXY_LIT("else") >>
                                                                     dsl::p<Predicate>);
        static constexpr auto value = lexy::callback<ast::IfThenElsePtr>(
            [](auto &&expr, auto &&pred1, auto &&pred2) {
                return std::make_unique<ast::IfThenElse>(std::move(expr), std::move(pred1),
                                                         std::move(pred2));
            });
    };

    struct Short
    {
        static constexpr auto rule = LEXY_LIT("if") >> dsl::p<ExpressionProduct> >>
                                     LEXY_LIT("then") >> dsl::p<Predicate>;
        static constexpr auto value = lexy::callback<ast::IfThenPtr>(
            [](auto &&expr, auto &&pred)
            { return std::make_unique<ast::IfThen>(std::move(expr), std::move(pred)); });
    };

    static constexpr auto rule = dsl::p<Full> | dsl::p<Short>;
    static constexpr auto value = lexy::callback<ast::СonditionPtr>(
        [](auto &&cond) { return std::make_unique<ast::Сondition>(std::move(cond)); });
};

struct BaseStatement : lexy::token_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule =
        (dsl::lookahead(LEXY_LIT("if"), dsl::lit_c<'}'>) >> dsl::p<Conditional>) |
        dsl::else_ >> dsl::p<Quantifier>;
    static constexpr auto value = lexy::callback<ast::BaseStatementPtr>(
        [](auto &&inner) { return std::make_unique<ast::BaseStatement>(std::move(inner)); });
};

struct Assignment
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::p<Identifier> >>
                                 dsl::while_(dsl::ascii::space) >> dsl::lit_c<'='> >>
                                 dsl::while_(dsl::ascii::space) >> dsl::p<ExpressionProduct>;

    static constexpr auto value = lexy::callback<ast::AssignmentStatementPtr>(
        [](auto &&name, auto &&expr)
        { return std::make_unique<ast::AssignmentStatement>(std::move(name), std::move(expr)); });
};

struct ExceptQuantifier
{
    static constexpr auto whitespace = dsl::ascii::space | dsl::ascii::newline;
    static constexpr auto rule = LEXY_LIT("except") >> dsl::p<Quantifier>;
    static constexpr auto value = lexy::callback<ast::ExceptStatementPtr>(
        [](auto &&inner) { return std::make_unique<ast::ExceptStatement>(std::move(inner)); });
};

struct BodyStatement
{
    static constexpr auto whitespace = dsl::ascii::space | dsl::ascii::newline;
    static constexpr auto rule =
        (dsl::lookahead(LEXY_LIT("except"), dsl::lit_c<';'>) >> dsl::p<ExceptQuantifier>) |
        ((dsl::lookahead(LEXY_LIT("all"), dsl::lit_c<';'>) |
          dsl::lookahead(LEXY_LIT("exist"), dsl::lit_c<';'>)) >>
         dsl::p<Quantifier>) |
        dsl::else_ >> dsl::p<Assignment>;
    static constexpr auto value = lexy::callback<ast::BodyStatementPtr>(
        [](auto &&inner) { return std::make_unique<ast::BodyStatement>(std::move(inner)); });
};
} // namespace lang::grammar