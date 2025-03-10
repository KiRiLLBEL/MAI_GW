#include <lang/ast.hpp>
#include <lang/expression.hpp>
#include <gtest/gtest.h>

#include <lang/parser.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>
#include <memory>
#include <vector>

template<typename T>
inline auto parseMultiple(const auto& arrayInput) -> void
{
    for(const auto& input: arrayInput)
    {
        const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
        const auto result = lexy::parse<T>(str_input, lexy_ext::report_error);
        EXPECT_TRUE(result.has_value());
    }
}

TEST(ParserTestSmoke, IdentifierSmoke)
{
    const std::string input{"hello_12_3"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::identifier>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
}

TEST(ParserTestSmoke, StringSmoke)
{
    const std::string input{R"("hello_12_3")"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::string>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("hello_12_3", result.value());
}

TEST(ParserTestSmoke, BooleanSmoke)
{
    const std::string inputTrue{"true"};
    const std::string inputFalse{"false"};
    auto str_input = lexy::string_input<lexy::utf8_encoding>(inputTrue);
    auto result = lexy::parse<lang::grammar::boolean>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(true, result.value());

    str_input = lexy::string_input<lexy::utf8_encoding>(inputFalse);
    result = lexy::parse<lang::grammar::boolean>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(false, result.value());
}

TEST(ParserTestSmoke, NumberSmoke)
{
    const std::string input{"145267"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::number>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(145267, result.value());
}

TEST(ParserTestSmoke, LiteralSimpleSmoke)
{
    static constexpr std::array<std::string, 3> literals = {
        "525",
        R"("test")",
        "true"
    };
    parseMultiple<lang::grammar::literal_simple>(literals);
}

TEST(ParserTestSmoke, SetSmoke)
{
    const std::string input{R"([145267, "ok"  , true])"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::set>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedVector = std::get<std::vector<lang::ast::ExpressionPtr>>(std::get<lang::ast::LiteralPtr>(*result.value())->value);
    EXPECT_EQ(parsedVector.size(), 3);
}

TEST(ParserTestSmoke, LiteralSmoke)
{
    static constexpr std::array<std::string_view, 5> literals = {
        "525",
        R"("test")",
        "true",
        R"([145267, "ok"  , true])",
        R"([145267])"
    };
    parseMultiple<lang::grammar::literal>(literals);
}

TEST(ParserTestSmoke, VariableSmoke)
{
    const std::string input{"test_name_132"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::variable>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedVariable = *std::get<lang::ast::VariablePtr>(*result.value());
    EXPECT_EQ(parsedVariable.name, input);
}

TEST(ParserTestSmoke, FunctionArgsSmoke)
{
    const std::string input{R"({525,   "test",   true, [145267]})"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::func_args>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedVector = result.value();
    EXPECT_EQ(parsedVector.size(), 4);
}

TEST(ParserTestSmoke, UnaryExprSmoke)
{
    const std::string input{"not true"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::expression>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedUnary = *std::get<lang::ast::UnaryExpPtr>(*result.value());
    const auto& parsedLiteal = *std::get<lang::ast::LiteralPtr>(*parsedUnary.operand);
    EXPECT_EQ(parsedUnary.op, lang::ast::ExprOpType::NEG);
    EXPECT_EQ(std::get<bool>(parsedLiteal.value), true);
}

TEST(ParserTestSmoke, PropertySmoke)
{
    const std::string input{"test.!op"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::expression>(str_input, lexy_ext::report_error);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedProperty = *std::get<lang::ast::AccessExprPtr>(*result.value());
    EXPECT_EQ(parsedProperty.prop, "op");
}

TEST(ParserTestSmoke, TernarySmoke)
{
    const std::string input{"true ? false : true"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::expression>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedProperty = *std::get<lang::ast::TernaryExprPtr>(*result.value());
    const auto& parsedLiteral = *std::get<lang::ast::LiteralPtr>(*parsedProperty.condition);
    EXPECT_EQ(std::get<bool>(parsedLiteral.value), true);
}

TEST(ParserTestSmoke, BinaryExprSmoke)
{
    const std::string input{"2 + 2"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::expression>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedProperty = *std::get<lang::ast::BinaryExprPtr>(*result.value());
    EXPECT_EQ(parsedProperty.op, ExprOpType::PLUS);
}

TEST(ParserTestSmoke, AssignmentSmoke)
{
    const std::string input{"x = 5"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::assignment>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    const auto& parsedProperty = *std::get<lang::ast::AssignmentStatementPtr>(*result.value());
    EXPECT_EQ(parsedProperty.name, "x");
}

TEST(ParserTestSmoke, QuantifierTokenSmoke)
{
    const std::string input{"exist"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::quantifier::quantifier_token_exist>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
    EXPECT_EQ(result.value(), QuantifierType::Exist);
}

TEST(ParserTestSmoke, QuantifierSmoke)
{
    const std::string input{R"(all { s1 in s: s1.tech in ["lst", "gif"] })"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::statement>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
}

TEST(ParserTestSmoke, ConditionalSmoke)
{
    const std::string input{R"(
    all { 
        s1 in s:
            if s1.tech > 10
                then exist 
                    { t1 in t: true } 
                else all 
                    { t2 in t: false}
    }
    )"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::statement>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
}


TEST(ParserTestSmoke, NestedQuantifierSmoke)
{
    const std::string input{R"(
    all { 
        s1 in s:
            "DMZ" in s1.props:
            exist {
                p in t:
                    p.x == true
            }
        }
    )"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::statement>(str_input, lexy_ext::report_error);
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
        s1 in s:
            "DMZ" in s1.props:
            exist {
                p in t:
                    p.x == true
            }
    };
    all { 
        s1 in s:
            "DMZ" in s1.props:
            exist {
                p in t:
                    p.x == true
            }
    };
    except exist {
        s1 in s:
            s1.tech in ["go"]
    }
    )"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::block>(str_input, lexy_ext::report_error);
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
            s1 in s:
                "DMZ" in s1.props:
                exist {
                    p in t:
                        p.x == true
                }
        };
        all { 
            s1 in s:
                "DMZ" in s1.props:
                exist {
                    p in t:
                        p.x == true
                }
        };
        except exist {
            s1 in s:
                s1.tech in ["go"]
        }
    }
    )"};
    const auto str_input = lexy::string_input<lexy::utf8_encoding>(input);
    const auto result = lexy::parse<lang::grammar::rule_decl>(str_input, lexy_ext::report_error);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.errors());
}