#pragma once

#include <lang/ast.hpp>
#include <lang/expression.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/forward.hpp>
#include <lexy/callback/object.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/ascii.hpp>
#include <lexy/dsl/brackets.hpp>
#include <lexy/dsl/branch.hpp>
#include <lexy/dsl/combination.hpp>
#include <lexy/dsl/digit.hpp>
#include <lexy/dsl/expression.hpp>
#include <lexy/dsl/identifier.hpp>
#include <lexy/dsl/integer.hpp>
#include <lexy/dsl/list.hpp>
#include <lexy/dsl/literal.hpp>
#include <lexy/dsl/loop.hpp>
#include <lexy/dsl/option.hpp>
#include <lexy/dsl/production.hpp>
#include <lexy/dsl/punctuator.hpp>
#include <lexy/dsl/recover.hpp>
#include <lexy/dsl/return.hpp>
#include <lexy/dsl/separator.hpp>
#include <lexy/dsl/whitespace.hpp>
#include <lexy/grammar.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>

#include <memory>
#include <string>
#include <vector>

namespace {
    using namespace lang::ast;

    auto CreateBinaryExprPtr(ExpressionPtr&& lhs, ExprOpType&& type, ExpressionPtr&& rhs)
    {
        return std::make_unique<Expression>(std::move(std::make_unique<BinaryExpr>(std::move(lhs), std::move(type), std::move(rhs))));
    }
}

namespace lang::grammar
{

namespace dsl = lexy::dsl;

struct ws : lexy::token_production
{
    static constexpr auto rule = dsl::whitespace(dsl::ascii::space);
    static constexpr auto value = lexy::noop;
};

struct identifier : lexy::token_production
{
    static constexpr auto rule = []{
            const auto id   = dsl::identifier(dsl::ascii::alpha_digit_underscore, dsl::ascii::alpha_digit_underscore);
            const auto kw_not = LEXY_KEYWORD("not", id);
            const auto kw_in = LEXY_KEYWORD("in", id);
            const auto kw_or = LEXY_KEYWORD("or", id);
            const auto kw_and = LEXY_KEYWORD("and", id);
            const auto kw_xor = LEXY_KEYWORD("xor", id);
            const auto kw_all = LEXY_KEYWORD("all", id);
            const auto kw_exist = LEXY_KEYWORD("exist", id);
            const auto kw_true = LEXY_KEYWORD("true", id);
            const auto kw_false = LEXY_KEYWORD("false", id);
            return id.reserve(kw_not, kw_in, kw_or, kw_and, kw_xor, kw_all, kw_exist, kw_true, kw_false);
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

struct string : lexy::token_production
{
    static constexpr auto rule  = dsl::quoted(dsl::code_point);
    static constexpr auto value = lexy::as_string<std::string>;
};

struct boolean : lexy::token_production
{
    struct true_m : lexy::transparent_production
    {
        static constexpr auto rule = LEXY_LIT("true");
        static constexpr auto value = lexy::constant(true);
    };

    struct false_m : lexy::transparent_production
    {
        static constexpr auto rule = LEXY_LIT("false");
        static constexpr auto value = lexy::constant(false);
    };

    static constexpr auto rule = dsl::p<true_m> | dsl::p<false_m>;
    static constexpr auto value = lexy::callback<bool>([](bool val) -> bool {return val;});
};

struct number : lexy::token_production
{
    static constexpr auto rule = dsl::integer<int64_t>;
    static constexpr auto value = lexy::forward<int64_t>;
};

struct literal_simple : lexy::token_production
{
    static constexpr auto rule = dsl::p<string>
    | dsl::p<boolean>
    | dsl::p<number>;

    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string&& str) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(
                ast::LiteralType::String,
                std::move(str)
            );

            return std::make_unique<ast::Expression>(std::move(lit));
        },
        [](bool b_val) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(
                ast::LiteralType::Bool,
                std::move(b_val)
            );
            return std::make_unique<ast::Expression>(std::move(lit));
        },
        [](int64_t num_val) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(
                ast::LiteralType::Number,
                num_val
            );
            return std::make_unique<ast::Expression>(std::move(lit));
        }
    );
};

struct set : lexy::token_production
{
    static constexpr auto rule =
        dsl::square_bracketed.list(
            dsl::p<literal_simple> >> dsl::while_(dsl::ascii::space), 
            dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space))
        );
    static constexpr auto value = lexy::as_list<std::vector<ast::ExpressionPtr>> >> lexy::callback<ast::ExpressionPtr>(
        [](std::vector<ast::ExpressionPtr>&& set_items) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(
                ast::LiteralType::Set,
                std::move(set_items)
            );
            return std::make_unique<ast::Expression>(std::move(lit));
        }
    );
};

struct literal : lexy::token_production
{
    static constexpr auto rule = dsl::p<set> | dsl::p<literal_simple>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct variable : lexy::token_production
{
    static constexpr auto rule = dsl::p<identifier>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string&& var_name) -> ast::ExpressionPtr
        {
            auto var = std::make_unique<ast::Variable>(
                std::move(var_name)
            );
            return std::make_unique<ast::Expression>(std::move(var));
        }
    );
};

struct func_args : lexy::token_production
{
    static constexpr auto rule =
        dsl::curly_bracketed.opt_list(dsl::p<literal> >> dsl::while_(dsl::ascii::space), dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));
    static constexpr auto value =
        lexy::as_list<std::vector<ast::ExpressionPtr>>;
};

struct function_call
{
    static constexpr auto rule = dsl::p<identifier> >> dsl::p<func_args>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string name, std::vector<ast::ExpressionPtr> args){
            auto fc = std::make_unique<ast::FunctionCall>(
                std::move(name),
                std::move(args)
            );
            return std::make_unique<ast::Expression>(std::move(fc));
        }
    );
};

struct nested_expr : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct expression>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct factor_expr
{
    static constexpr auto rule = dsl::parenthesized(dsl::p<nested_expr>) | dsl::p<literal> | dsl::p<variable> | dsl::p<function_call>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct expression : lexy::expression_production
{
    static constexpr auto whitespace= dsl::ascii::space;
    static constexpr auto atom = dsl::p<factor_expr>;

    static constexpr auto op_plus  = dsl::op(dsl::lit_c<'+'>);
    static constexpr auto op_minus = dsl::op(dsl::lit_c<'-'>);
    static constexpr auto op_mult = dsl::op(dsl::lit_c<'*'>);
    static constexpr auto op_div = dsl::op(dsl::lit_c<'/'>);
    static constexpr auto op_neg = dsl::op(LEXY_LIT("not"));
    static constexpr auto op_equal = dsl::op(LEXY_LIT("=="));
    static constexpr auto op_not_equal = dsl::op(LEXY_LIT("/="));
    static constexpr auto op_less = dsl::op(dsl::lit_c<'<'>);
    static constexpr auto op_greater = dsl::op(dsl::lit_c<'>'>);
    static constexpr auto op_less_eq = dsl::op(LEXY_LIT("<="));
    static constexpr auto op_greater_eq = dsl::op(LEXY_LIT(">="));
    static constexpr auto op_in = dsl::op(LEXY_LIT("in"));
    static constexpr auto op_and = dsl::op(LEXY_LIT("and"));
    static constexpr auto op_or = dsl::op(LEXY_LIT("or"));
    static constexpr auto op_xor = dsl::op(LEXY_LIT("xor"));

    struct access_expr : dsl::postfix_op
    {
        static constexpr auto op = dsl::op<false>(LEXY_LIT(".") >> dsl::p<identifier>) / dsl::op<true>(LEXY_LIT(".!") >> dsl::p<identifier>);
        using operand = dsl::atom;
    };

    struct unary_expr : dsl::prefix_op
    {
        static constexpr auto op = op_neg;
        using operand = access_expr;
    };

    struct mult_expr : dsl::infix_op_left
    {
        static constexpr auto op = op_mult / op_div;
        using operand = unary_expr;
    };

    struct add_expr : dsl::infix_op_left
    {
        static constexpr auto op = op_plus / op_minus;
        using operand = mult_expr;
    };

    struct compare_expr : dsl::infix_op_left
    {
        static constexpr auto op = op_equal / op_not_equal / op_greater / op_less / op_greater_eq / op_less_eq / op_in;
        using operand = add_expr;
    };

    struct logical_expr : dsl::infix_op_left
    {
        static constexpr auto op = op_and / op_xor / op_or;
        using operand = compare_expr;
    };

    struct ternary_expr : dsl::infix_op_single
    {
        static constexpr auto op = dsl::op<void>(LEXY_LIT("?") >> dsl::p<nested_expr> + dsl::lit_c<':'>);
        using operand = logical_expr;
    };

    using operation = ternary_expr;

    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](ast::ExpressionPtr&& expr) { return expr; },
        [](lexy::op<op_neg>, ast::ExpressionPtr&& expr) { return std::make_unique<ast::Expression>(std::make_unique<ast::UnaryExpr>(ExprOpType::NEG, std::move(expr))); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_mult>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::MULT, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_div>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::DIV, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_plus>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::PLUS, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_minus>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::MINUS, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_equal>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::EQ, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_not_equal>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::NOT_EQ, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_greater>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::GREATER, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_less>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::LESS, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_greater_eq>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::GREATER_EQ, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_less_eq>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::LESS_EQ, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_in>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::IN, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_and>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::AND, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_xor>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::XOR, std::move(rhs)); },
        [](ast::ExpressionPtr&& lhs, lexy::op<op_or>, ast::ExpressionPtr&& rhs) { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::OR, std::move(rhs)); },
        [](ast::ExpressionPtr&& if_, ast::ExpressionPtr&& then_, ast::ExpressionPtr&& else_) { return std::make_unique<Expression>(std::move(std::make_unique<ast::TernaryExpr>(std::move(if_), std::move(then_), std::move(else_)))); },
        [](ast::ExpressionPtr&& var, bool isSafe, std::string prop) { return std::make_unique<Expression>(std::move(std::make_unique<ast::AccessExpr>(std::move(var), isSafe ? ExprOpType::SAFE_ACCESS : ExprOpType::ACCESS, std::move(prop)))); }
    );
};

struct nested_statement : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct statement>;
    static constexpr auto value = lexy::forward<ast::StatementPtr>;
};

struct statement
{
    struct assignment
    {
        static constexpr auto rule = dsl::p<identifier> 
            >> dsl::while_(dsl::ascii::space) 
            >> dsl::lit_c<'='> 
            >> dsl::while_(dsl::ascii::space) 
            >> dsl::p<expression>;
        static constexpr auto value = lexy::callback<ast::StatementPtr>(
            [](std::string&& name, ast::ExpressionPtr&& expr) -> ast::StatementPtr
            {
                auto asg = std::make_unique<ast::AssignmentStatement>(
                    std::move(name),
                    std::move(expr)
                );
                return std::make_unique<ast::Statement>(std::move(asg));
            }
        );
    };

    struct quantifier
    {
        struct quantifier_token_all
        {
            static constexpr auto rule = LEXY_LIT("all");
            static constexpr auto value = lexy::constant(ast::QuantifierType::All);
        };

        struct quantifier_token_exist
        {
            static constexpr auto rule = LEXY_LIT("exist");
            static constexpr auto value = lexy::constant(ast::QuantifierType::Exist);
        };

        static constexpr auto rule = (dsl::p<quantifier_token_all> | dsl::p<quantifier_token_exist>)
            >> dsl::while_(dsl::ascii::space | dsl::ascii::newline)
            >> dsl::curly_bracketed(dsl::p<nested_statement>);

        static constexpr auto value = lexy::callback<ast::StatementPtr>(
            [](ast::QuantifierType&& type, ast::StatementPtr&& block) -> ast::StatementPtr
            {
                auto q = std::make_unique<ast::QuantifierStatement>(
                    type,
                    std::move(block)
                );
                return std::make_unique<ast::Statement>(std::move(q));
            }
        );
    };

    struct selection
    {
        struct selection_list
        {
            static constexpr auto rule = dsl::list(
                dsl::p<identifier> >> dsl::while_(dsl::ascii::space), 
                dsl::sep(dsl::comma >> dsl::ascii::space));

            static constexpr auto value = lexy::as_list<std::vector<std::string>>;
        };

        static constexpr auto rule = dsl::p<selection_list>
            >> dsl::while_(dsl::ascii::space)
            >> LEXY_LIT("in")
            >> dsl::while_(dsl::ascii::space)
            >> dsl::p<expression>
            >> dsl::while_(dsl::ascii::space)
            >> dsl::lit_c<':'>
            >> dsl::while_(dsl::ascii::space | dsl::ascii::newline)
            >> dsl::p<nested_statement>;

        static constexpr auto value = lexy::callback<ast::StatementPtr>(
            [](std::vector<std::string>&& varNames, ast::ExpressionPtr collection, ast::StatementPtr body) -> ast::StatementPtr{
                auto sel = std::make_unique<ast::SelectionStatement>(
                    std::move(varNames),
                    std::move(collection),
                    std::move(body)
                );
                return std::make_unique<ast::Statement>(std::move(sel));
            }
        );
    };

    static constexpr auto rule = 
        dsl::p<assignment> |
        dsl::p<quantifier> |
        dsl::p<selection>;

    static constexpr auto value = lexy::forward<ast::StatementPtr>;
};

struct nested_block : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct block>;
    static constexpr auto value = lexy::forward<ast::BlockPtr>;
};

struct block
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto rule = dsl::list(dsl::p<statement>);
    
    static constexpr auto value = lexy::as_list<std::vector<ast::StatementPtr>> >> lexy::callback<ast::BlockPtr>(
        [](std::vector<ast::StatementPtr>&& stmts) -> ast::BlockPtr {
            return std::make_unique<ast::Block>(
                std::move(stmts)
            );
        }
    );
};

// struct except_block
// {
//     static constexpr auto rule = 
//         LEXY_LIT("except") >> dsl::curly_bracketed(dsl::list(dsl::p<block>));
//     static constexpr auto value = lexy::forward<ast::BlockPtr>;
// };

// struct rule_decl
// {
//     static constexpr auto rule =
//         LEXY_LIT("rule") 
//         >> dsl::p<string>
//         >> dsl::curly_bracketed
//         (
//             dsl::p<block> + dsl::list(dsl::p<except_block>)
//         );

//     static constexpr auto value = lexy::callback<ast::Rule>(
//         [](std::string name, ast::BlockPtr mainBlock, std::vector<ast::BlockPtr> excepts)
//         {
//             ast::Rule r;
//             r.name = std::move(name);
//             r.mainBlock = std::move(mainBlock);
//             r.exceptBlocks = std::move(excepts);
//             return r;
//         }
//     );
// };

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

}