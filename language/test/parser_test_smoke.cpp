#include <ast/ast.hpp>
#include <ast/expression.hpp>
#include <lexy/action/parse.hpp>
#include <parser/expressions.hpp>
#include <parser/identifiers.hpp>
#include <parser/literals.hpp>
#include <parser/parser.hpp>
#include <parser/statements.hpp>

#include <gtest/gtest.h>

#include <lexy/encoding.hpp>
#include <lexy/input/string_input.hpp>

#include <lexy_ext/report_error.hpp>

#include <memory>
#include <vector>

template <typename T> inline auto ParseMultiple(const auto &arrayInput) -> void
{
    for (const auto &input : arrayInput)
    {
        const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
        const auto result = lexy::parse<T>(strInput, lexy_ext::report_error);
        EXPECT_TRUE(result.has_value());
    }
}

TEST(ParserTestSmoke, IdentifierSmoke)
{
    const std::string input{"hello_12_3"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Identifier>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
}

TEST(ParserTestSmoke, StringSmoke)
{
    const std::string input{R"("hello_12_3")"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::String>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("hello_12_3", result.value());
}

TEST(ParserTestSmoke, BooleanSmoke)
{
    const std::string inputTrue{"true"};
    const std::string inputFalse{"false"};
    auto strInput = lexy::string_input<lexy::utf8_encoding>(inputTrue);
    auto result = lexy::parse<lang::grammar::Boolean>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(true, result.value());

    strInput = lexy::string_input<lexy::utf8_encoding>(inputFalse);
    result = lexy::parse<lang::grammar::Boolean>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(false, result.value());
}

TEST(ParserTestSmoke, NumberSmoke)
{
    const std::string input{"145267"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Number>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(145267, result.value());
}

TEST(ParserTestSmoke, LiteralSimpleSmoke)
{
    static constexpr std::array<std::string, 3> literals = {"525", R"("test")", "true"};
    ParseMultiple<lang::grammar::LiteralSimple>(literals);
}

TEST(ParserTestSmoke, SetSmoke)
{
    const std::string input{R"([145267, "ok"  , true])"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Set>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedVector = std::get<lang::ast::SetPtr>(*result.value())->items;
    EXPECT_EQ(parsedVector.size(), 3);
}

TEST(ParserTestSmoke, LiteralSmoke)
{
    static constexpr std::array<std::string_view, 5> literals = {
        "525", R"("test")", "true", R"([145267, "ok"  , true])", R"([145267])"};
    ParseMultiple<lang::grammar::Literal>(literals);
}

TEST(ParserTestSmoke, VariableSmoke)
{
    const std::string input{"test_name_132"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = parse<lang::grammar::Variable>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedVariable = *std::get<lang::ast::VariablePtr>(*result.value());
    EXPECT_EQ(parsedVariable.name, input);
}

TEST(ParserTestSmoke, FunctionArgsSmoke)
{
    const std::string input{R"((525,   "test",   true, [145267]))"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::FuncArgs>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedVector = result.value();
    EXPECT_EQ(parsedVector.size(), 4);
}

TEST(ParserTestSmoke, KeywordSmoke)
{
    const std::string input{"system"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Keyword>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
}

TEST(ParserTestSmoke, UnaryExprSmoke)
{
    const std::string input{"not true"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedUnary = *std::get<lang::ast::NegationPtr>(*result.value());
    const auto &parsedLiteal = *std::get<lang::ast::BoolPtr>(*parsedUnary.operand);
    EXPECT_EQ(parsedLiteal.value, true);
}

TEST(ParserTestSmoke, FunctionCallSmoke)
{
    const std::string input{"route(s1, s2)"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedProperty = *std::get<lang::ast::CallPtr>(*result.value());
    EXPECT_EQ(parsedProperty.functionName, "route");
}

TEST(ParserTestSmoke, PropertySmoke)
{
    const std::string input{"test.!op"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedProperty = *std::get<lang::ast::SafeAccessExprPtr>(*result.value());
    EXPECT_EQ(parsedProperty.prop, "op");
}

TEST(ParserTestSmoke, TernarySmoke)
{
    const std::string input{"true ? false : true"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedProperty = *std::get<lang::ast::TernaryExprPtr>(*result.value());
    const auto &parsedLiteral = *std::get<lang::ast::BoolPtr>(*parsedProperty.condition);
    EXPECT_EQ(parsedLiteral.value, true);
}

TEST(ParserTestSmoke, BinaryExprSmoke)
{
    const std::string input{"2 + 2"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result =
        lexy::parse<lang::grammar::ExpressionProduct>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    std::get<lang::ast::AddPtr>(*result.value());
}

TEST(ParserTestSmoke, AssignmentSmoke)
{
    const std::string input{"x = 5"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::BodyStatement>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto &parsedProperty = *std::get<lang::ast::AssignmentStatementPtr>(*result.value());
    EXPECT_EQ(parsedProperty.name, "x");
}

TEST(ParserTestSmoke, QuantifierSmoke)
{
    const std::string input{R"(all { s1 in system: s1.tech in ["lst", "gif"] })"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Quantifier>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
}

TEST(ParserTestSmoke, ConditionalSmoke)
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
    EXPECT_FALSE(result.errors());
}

TEST(ParserTestSmoke, NestedQuantifierSmoke)
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
    EXPECT_FALSE(result.errors());
}

TEST(ParserTestSmoke, BlockSmoke)
{
    const std::string input{R"(
    x = 10;
    y = true;
    z = ["token", 1, true];
    all {
        s1 in component:
            "DMZ" in s1.props:
            exist {
                p in code:
                    p.x == true
            }
    };
    all {
        s1 in deploy:
            "DMZ" in s1.props:
            exist {
                p in code:
                    p.x == true
            }
    };
    except exist {
        s1 in code:
            s1.tech in ["go"]
    }
    )"};
    const auto strInput = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::Block>(strInput, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
}

TEST(ParserTestSmoke, RuleSmoke)
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
    EXPECT_FALSE(result.errors());
}