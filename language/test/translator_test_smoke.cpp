#include "lang/ast.hpp"
#include "translator/context.hpp"
#include <translator/translator.hpp>

#include <lang/parser.hpp>

#include <gtest/gtest.h>

TEST(TranslatorTestSmoke, RuleOptionsSmoke)
{
    const std::string input{R"(rule first {
        description: "Hello world";
        priority: Info;
        x = 10
    }
    )"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::rule_decl>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    cypher::Translator translator;
    const auto translation = translator.translateRule(result.value());
    EXPECT_TRUE(not translation.empty());
}

TEST(TranslatorTestSmoke, SelectionSmoke)
{
    const std::string input{R"(exist {s1, s2 in system: s1.test > s2.tech})"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::quantifier>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    cypher::Translator translator;
    cypher::Context context;
    const auto translation = translator.translateQuantifierStatement(context, std::get<lang::ast::QuantifierStatementPtr>(*result.value()));
    EXPECT_TRUE(not translation.empty());
}

TEST(TranslatorTestSmoke, AssigmentSmoke)
{
    const std::string input{R"(x = 5)"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::assignment>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    cypher::Translator translator;
    cypher::Context context;
    const auto translation = translator.translateAssignmentStatement(context, std::get<lang::ast::AssignmentStatementPtr>(*result.value()));
    EXPECT_TRUE(not translation.empty());
}