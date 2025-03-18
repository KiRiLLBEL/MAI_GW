#include <lang/ast.hpp>
#include <translator/context.hpp>
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
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::RuleDecl>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto translation = cypher::Translator::TranslateRule(result.value());
    EXPECT_TRUE(not translation.empty());
}

TEST(TranslatorTestSmoke, SelectionSmoke)
{
    const std::string input{R"(exist {s1, s2 in system: s1.test > s2.tech})"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Quantifier>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    cypher::Context context;
    const auto translation = cypher::Translator::TranslateQuantifierStatement(
        context, std::get<lang::ast::QuantifierStatementPtr>(*result.value()));
    EXPECT_TRUE(not translation.empty());
}

TEST(TranslatorTestSmoke, AssigmentSmoke)
{
    const std::string input{R"(x = 5)"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Assignment>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    cypher::Context context;
    const auto translation = cypher::Translator::TranslateAssignmentStatement(
        context, std::get<lang::ast::AssignmentStatementPtr>(*result.value()));
    EXPECT_TRUE(not translation.empty());
}