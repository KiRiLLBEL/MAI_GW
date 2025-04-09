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
    const auto result = lang::grammar::ParseTest<lang::grammar::Keyword>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, SetSmoke)
{
    const std::string input{R"([134, true, "hello"])"};
    const auto result = lang::grammar::ParseTest<lang::grammar::Set>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, VariableSmoke)
{
    const std::string input{"Hello"};
    const auto result = lang::grammar::ParseTest<lang::grammar::IdentifierExpr>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, FunctionArgsSmoke)
{
    const std::string input{"testing_funct(2, hello, [123, true])"};
    const auto result = lang::grammar::ParseTest<lang::grammar::ExpressionProduct>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, TernarySmoke)
{
    const std::string input{"true ? false : true"};
    const auto result = lang::grammar::ParseTest<lang::grammar::ExpressionProduct>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, AddSmoke)
{
    const std::string input{"2 + 4"};
    const auto result = lang::grammar::ParseTest<lang::grammar::ExpressionProduct>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, AssignmentSmoke)
{
    const std::string input{"x = 5"};
    const auto result = lang::grammar::ParseTest<lang::grammar::BodyStatement>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

TEST(TestJsonSmoke, QuantifierSmoke)
{
    const std::string input{R"(all { s1 in system: s1.tech in ["lst", "gif"] })"};
    const auto result = lang::grammar::ParseTest<lang::grammar::Quantifier>(input);
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
    const auto result = lang::grammar::ParseTest<lang::grammar::Quantifier>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}

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
    const auto result = lang::grammar::ParseTest<lang::grammar::RuleDecl>(input);
    EXPECT_TRUE(result.has_value());
    const auto jsonResult = lang::ast::json::Serialize(result.value());
    EXPECT_TRUE(not jsonResult.empty());
    GTEST_LOG_(INFO) << jsonResult;
}