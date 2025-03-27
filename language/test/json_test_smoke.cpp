#include "parser/expressions.hpp"
#include "parser/literals.hpp"
#include "parser/parser.hpp"
#include "parser/statements.hpp"
#include <gtest/gtest.h>
#include <json/serializer.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>
#include <parser/identifiers.hpp>
#include <string>

TEST(TestJsonSmoke, IdentifierSmoke)
{
    const std::string input{"system"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Keyword>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, SetSmoke)
{
    const std::string input{R"([134, true, "hello"])"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Set>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, VariableSmoke)
{
    const std::string input{"Hello"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Variable>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, FunctionArgsSmoke)
{
    const std::string input{"testing_funct{2, hello, [123, true]}"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::FunctionCall>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, TernarySmoke)
{
    const std::string input{"true ? false : true"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, AssignmentSmoke)
{
    const std::string input{"x = 5"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::BodyStatement>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, QuantifierSmoke)
{
    const std::string input{R"(all { s1 in system: s1.tech in ["lst", "gif"] })"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Quantifier>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, ConditionalSmoke)
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
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

// TODO: binary expression fix
TEST(TestJsonSmoke, RuleSmoke)
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
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}