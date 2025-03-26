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
}