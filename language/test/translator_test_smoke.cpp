#include "parser/expressions.hpp"
#include "parser/identifiers.hpp"
#include "parser/literals.hpp"
#include "parser/statements.hpp"
#include <ast/ast.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>
#include <translator/translator.hpp>

#include <parser/parser.hpp>

#include <gtest/gtest.h>

TEST(TranslatorTestSmoke, LiteralSmoke)
{
    const std::string input{"100"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Literal>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, KeywordSmoke)
{
    const std::string input{"system"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Keyword>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, SetSmoke)
{
    const std::string input{R"([123, "hello world", true])"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Set>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, VariableSmoke)
{
    const std::string input{R"(test)"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_ANY_THROW({ const auto translation = lang::ast::cypher::Translate(result.value()); });
}

TEST(TranslatorTestSmoke, AccessSmoke)
{
    const std::string input{R"(system.!test)"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, FunctionCallSmoke)
{
    const std::string input{R"(route(1, 1))"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, AssignmentSmoke)
{
    const std::string input{R"(x = 10 + 1)"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Assignment>(strInput, lexy_ext::report_error);
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, QuantifierSmoke)
{
    const std::string input{R"(all { s1, s2, s3 in system: s1.tech in ["Go", "JS"]})"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Quantifier>(strInput, lexy_ext::report_error);
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, IfThenSmoke)
{
    const std::string input{R"(
        all {
            s1 in system:
                if s1.tech > 10
                    then exist
                        { t1 in container: true }
                    else all
                        { t2 in container: false}
        }
        )"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Quantifier>(strInput, lexy_ext::report_error);
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, NestedQuantifierSmoke)
{
    const std::string input{R"(
    all {
        s1 in system:
            "DMZ" in s1.props:
            exist {
                p in container:
                    p.x == true
            }
        }
    )"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Quantifier>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, RuleSmoke)
{
    const std::string input{R"(rule first {
        description: "Hello world";
        priority: Info;
        x = 10;
        y = true;
        z = ["token", 1, true];
        all {
            s1 in system:
                "DMZ" in s1.props:
                exist {
                    p in container:
                        p.x == true
                }
        };
        except exist {
            s1 in system:
                s1.tech in ["go"]
        }
    }
    )"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::RuleDecl>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, HoldSmoke)
{
    const std::string input{R"(rule HOLD {
    description: "Hello world";
    priority: Info;
    lst = ["JS"];
    all {
        c in container:
            cross(c.tech, lst) == none
    }
    }
    )"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::RuleDecl>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}

TEST(TranslatorTestSmoke, DMZSmoke)
{
    const std::string input{R"(rule DMZ {
    description: "Hello world";
    priority: Info;
    exist {
        d in deploy:
            "DMZ" == d.name:
            exist { 
                c in d: "Database" in c.tags and c == none
            }
        }
    }
    )"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::RuleDecl>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto translation = lang::ast::cypher::Translate(result.value());
    EXPECT_TRUE(not translation.empty());
    GTEST_LOG_(INFO) << translation;
}