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
#include <optional>
#include <string>
#include <vector>

namespace
{
using namespace lang::ast;

auto CreateBinaryExprPtr(ExpressionPtr &&lhs, ExprOpType &&type, ExpressionPtr &&rhs)
{
    return std::make_unique<Expression>(
        std::make_unique<BinaryExpr>(std::move(lhs), type, std::move(rhs)));
}
} // namespace

namespace lang::grammar
{

namespace dsl = lexy::dsl;

struct Ws : lexy::token_production
{
    static constexpr auto rule = dsl::whitespace(dsl::ascii::space);
    static constexpr auto value = lexy::noop;
};

struct Identifier : lexy::token_production
{
    static constexpr auto rule = []
    {
        constexpr auto id =
            dsl::identifier(dsl::ascii::alpha_digit_underscore, dsl::ascii::alpha_digit_underscore);
        return id.reserve(LEXY_KEYWORD("not", id), LEXY_KEYWORD("in", id), LEXY_KEYWORD("or", id),
                          LEXY_KEYWORD("and", id), LEXY_KEYWORD("xor", id), LEXY_KEYWORD("all", id),
                          LEXY_KEYWORD("exist", id), LEXY_KEYWORD("true", id),
                          LEXY_KEYWORD("false", id), LEXY_KEYWORD("if", id),
                          LEXY_KEYWORD("then", id), LEXY_KEYWORD("else", id),
                          LEXY_KEYWORD("none", id), LEXY_KEYWORD("except", id),
                          LEXY_KEYWORD("priority", id), LEXY_KEYWORD("description", id));
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

struct String : lexy::token_production
{
    static constexpr auto rule = dsl::quoted(dsl::code_point);
    static constexpr auto value = lexy::as_string<std::string>;
};

struct Boolean : lexy::token_production
{
    struct TrueImpl : lexy::transparent_production
    {
        static constexpr auto rule = LEXY_LIT("true");
        static constexpr auto value = lexy::constant(true);
    };

    struct FalseImpl : lexy::transparent_production
    {
        static constexpr auto rule = LEXY_LIT("false");
        static constexpr auto value = lexy::constant(false);
    };

    static constexpr auto rule = dsl::p<TrueImpl> | dsl::p<FalseImpl>;
    static constexpr auto value = lexy::callback<bool>([](bool val) -> bool { return val; });
};

struct Number : lexy::token_production
{
    static constexpr auto rule = dsl::integer<int64_t>;
    static constexpr auto value = lexy::forward<int64_t>;
};

struct LiteralSimple : lexy::token_production
{
    static constexpr auto rule = dsl::p<String> | dsl::p<Boolean> | dsl::p<Number>;

    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string &&str) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(std::move(str));

            return std::make_unique<ast::Expression>(std::move(lit));
        },
        [](bool bVal) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(bVal);
            return std::make_unique<ast::Expression>(std::move(lit));
        },
        [](int64_t numVal) -> ast::ExpressionPtr
        {
            auto lit = std::make_unique<ast::Literal>(numVal);
            return std::make_unique<ast::Expression>(std::move(lit));
        });
};

struct Set : lexy::token_production
{
    static constexpr auto rule =
        dsl::square_bracketed.list(dsl::p<LiteralSimple> >> dsl::while_(dsl::ascii::space),
                                   dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));
    static constexpr auto
        value = lexy::as_list<std::vector<ast::ExpressionPtr>> >>
                lexy::callback<ast::ExpressionPtr>(
                    [](std::vector<ast::ExpressionPtr> &&setItems) -> ast::ExpressionPtr
                    {
                        auto lit = std::make_unique<ast::Literal>(std::move(setItems));
                        return std::make_unique<ast::Expression>(std::move(lit));
                    });
};

struct Literal : lexy::token_production
{
    static constexpr auto rule = dsl::p<Set> | dsl::p<LiteralSimple>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct Variable : lexy::token_production
{
    static constexpr auto rule = dsl::p<Identifier>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string &&varName) -> ast::ExpressionPtr
        {
            auto var = std::make_unique<ast::Variable>(std::move(varName));
            return std::make_unique<ast::Expression>(std::move(var));
        });
};

struct FuncArgs : lexy::token_production
{
    static constexpr auto rule =
        dsl::curly_bracketed.opt_list(dsl::p<Literal> >> dsl::while_(dsl::ascii::space),
                                      dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));
    static constexpr auto value = lexy::as_list<std::vector<ast::ExpressionPtr>>;
};

struct FunctionCall
{
    static constexpr auto rule = dsl::p<Identifier> >> dsl::p<FuncArgs>;
    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](std::string name, std::vector<ast::ExpressionPtr> args)
        {
            auto funcCall = std::make_unique<ast::FunctionCall>(std::move(name), std::move(args));
            return std::make_unique<ast::Expression>(std::move(funcCall));
        });
};

struct NestedExpr : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct ExpressionProduct>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct FactorExpr
{
    static constexpr auto rule = dsl::parenthesized(dsl::p<NestedExpr>) | dsl::p<Literal> |
                                 dsl::p<Variable> | dsl::p<FunctionCall>;
    static constexpr auto value = lexy::forward<ast::ExpressionPtr>;
};

struct ExpressionProduct : lexy::expression_production
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto atom = dsl::p<FactorExpr>;

    static constexpr auto opPlus = dsl::op(dsl::lit_c<'+'>);
    static constexpr auto opMinus = dsl::op(dsl::lit_c<'-'>);
    static constexpr auto opMult = dsl::op(dsl::lit_c<'*'>);
    static constexpr auto opDiv = dsl::op(dsl::lit_c<'/'>);
    static constexpr auto opNeg = dsl::op(LEXY_LIT("not"));
    static constexpr auto opEqual = dsl::op(LEXY_LIT("=="));
    static constexpr auto opNotEqual = dsl::op(LEXY_LIT("/="));
    static constexpr auto opLess = dsl::op(dsl::lit_c<'<'>);
    static constexpr auto opGreater = dsl::op(dsl::lit_c<'>'>);
    static constexpr auto opLessEq = dsl::op(LEXY_LIT("<="));
    static constexpr auto opGreaterEq = dsl::op(LEXY_LIT(">="));
    static constexpr auto opIn = dsl::op(LEXY_LIT("in"));
    static constexpr auto opAnd = dsl::op(LEXY_LIT("and"));
    static constexpr auto opOr = dsl::op(LEXY_LIT("or"));
    static constexpr auto opXor = dsl::op(LEXY_LIT("xor"));

    struct AccessExpr : dsl::postfix_op
    {
        static constexpr auto op = dsl::op<false>(LEXY_LIT(".") >> dsl::p<Identifier>) /
                                   dsl::op<true>(LEXY_LIT(".!") >> dsl::p<Identifier>);
        using operand = dsl::atom;
    };

    struct UnaryExpr : dsl::prefix_op
    {
        static constexpr auto op = opNeg;
        using operand = AccessExpr;
    };

    struct MultExpr : dsl::infix_op_left
    {
        static constexpr auto op = opMult / opDiv;
        using operand = UnaryExpr;
    };

    struct AddExpr : dsl::infix_op_left
    {
        static constexpr auto op = opPlus / opMinus;
        using operand = MultExpr;
    };

    struct CompareExpr : dsl::infix_op_left
    {
        static constexpr auto op =
            opEqual / opNotEqual / opGreater / opLess / opGreaterEq / opLessEq / opIn;
        using operand = AddExpr;
    };

    struct LogicalExpr : dsl::infix_op_left
    {
        static constexpr auto op = opAnd / opXor / opOr;
        using operand = CompareExpr;
    };

    struct TernaryExpr : dsl::infix_op_single
    {
        static constexpr auto op =
            dsl::op<void>(LEXY_LIT("?") >> (dsl::p<NestedExpr> + dsl::lit_c<':'>));
        using operand = LogicalExpr;
    };

    using operation = TernaryExpr;

    static constexpr auto value = lexy::callback<ast::ExpressionPtr>(
        [](ast::ExpressionPtr &&expr) { return expr; },
        [](lexy::op<opNeg>, ast::ExpressionPtr &&expr)
        {
            return std::make_unique<ast::Expression>(
                std::make_unique<ast::UnaryExpr>(ExprOpType::NEG, std::move(expr)));
        },
        [](ast::ExpressionPtr &&lhs, lexy::op<opMult>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::MULT, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opDiv>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::DIV, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opPlus>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::PLUS, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opMinus>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::MINUS, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opEqual>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::EQ, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opNotEqual>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::NOT_EQ, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opGreater>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::GREATER, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opLess>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::LESS, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opGreaterEq>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::GREATER_EQ, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opLessEq>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::LESS_EQ, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opIn>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::IN, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opAnd>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::AND, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opXor>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::XOR, std::move(rhs)); },
        [](ast::ExpressionPtr &&lhs, lexy::op<opOr>, ast::ExpressionPtr &&rhs)
        { return CreateBinaryExprPtr(std::move(lhs), ExprOpType::OR, std::move(rhs)); },
        [](ast::ExpressionPtr &&ifExpr, ast::ExpressionPtr &&thenExpr,
           ast::ExpressionPtr &&elseExpr)
        {
            return std::make_unique<ast::Expression>(std::make_unique<ast::TernaryExpr>(
                std::move(ifExpr), std::move(thenExpr), std::move(elseExpr)));
        },
        [](ast::ExpressionPtr &&var, bool isSafe, std::string prop)
        {
            return std::make_unique<ast::Expression>(std::make_unique<ast::AccessExpr>(
                std::move(var), isSafe ? ExprOpType::SAFE_ACCESS : ExprOpType::ACCESS,
                std::move(prop)));
        });
};

struct NestedSelection : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct Selection>;
    static constexpr auto value = lexy::forward<ast::SelectionStatementPtr>;
};

struct NestedQuantifier : lexy::transparent_production
{
    static constexpr auto whitespace = dsl::ascii::newline | dsl::ascii::space;
    static constexpr auto rule = dsl::recurse<struct Quantifier>;
    static constexpr auto value = lexy::forward<ast::StatementPtr>;
};

struct Quantifier
{
    struct QuantifierTokenAll
    {
        static constexpr auto rule = LEXY_LIT("all");
        static constexpr auto value = lexy::constant(ast::QuantifierType::ALL);
    };

    struct QuantifierTokenExist
    {
        static constexpr auto rule = LEXY_LIT("exist");
        static constexpr auto value = lexy::constant(ast::QuantifierType::ANY);
    };

    static constexpr auto rule = (dsl::p<QuantifierTokenAll> | dsl::p<QuantifierTokenExist>) >>
                                 dsl::while_(dsl::ascii::space | dsl::ascii::newline) >>
                                 dsl::curly_bracketed(dsl::p<NestedSelection>);

    static constexpr auto value = lexy::callback<ast::StatementPtr>(
        [](ast::QuantifierType &&type, ast::SelectionStatementPtr &&block) -> ast::StatementPtr
        {
            return std::make_unique<ast::Statement>(
                std::make_unique<ast::QuantifierStatement>(type, std::move(block)));
        });
};

struct Conditional
{
    static constexpr auto rule =
        LEXY_LIT("if") >> dsl::p<ExpressionProduct> >>
        LEXY_LIT("then") >> dsl::p<Quantifier> >> dsl::opt(LEXY_LIT("else") >> dsl::p<Quantifier>);

    static constexpr auto value = lexy::callback<ast::StatementPtr>(
        [](ast::ExpressionPtr &&cond, ast::StatementPtr &&then,
           lexy::nullopt &&) -> ast::StatementPtr
        {
            return std::make_unique<ast::Statement>(
                std::make_unique<ast::ConditionalStatement>(std::move(cond), std::move(then)));
        },
        [](ast::ExpressionPtr &&cond, ast::StatementPtr &&then,
           ast::StatementPtr &&elseExpr) -> ast::StatementPtr
        {
            return std::make_unique<ast::Statement>(std::make_unique<ast::ConditionalStatement>(
                std::move(cond), std::move(then), std::move(elseExpr)));
        });
};

struct Selection
{
    struct SelectionList
    {
        static constexpr auto rule =
            dsl::list(dsl::p<Identifier> >> dsl::while_(dsl::ascii::space),
                      dsl::sep(dsl::comma >> dsl::while_(dsl::ascii::space)));

        static constexpr auto value = lexy::as_list<std::vector<std::string>>;
    };

    static constexpr auto rule =
        dsl::p<SelectionList> >> dsl::while_(dsl::ascii::space) >>
        LEXY_LIT("in") >> dsl::while_(dsl::ascii::space) >> dsl::p<ExpressionProduct> >>
        dsl::while_(dsl::ascii::space) >> dsl::lit_c<':'> >> dsl::while_(dsl::ascii::space |
                                                                         dsl::ascii::newline) >>
        (dsl::p<Quantifier> | dsl::p<Conditional> |
         (dsl::else_ >> dsl::p<ExpressionProduct> >>
          dsl::opt(LEXY_LIT(":") >> dsl::p<Quantifier>)));

    static constexpr auto value = lexy::callback<ast::SelectionStatementPtr>(
        [](std::vector<std::string> &&varNames, ast::ExpressionPtr &&collection,
           ast::StatementPtr &&body) -> ast::SelectionStatementPtr
        {
            return std::make_unique<ast::SelectionStatement>(
                std::move(varNames), std::move(collection), std::move(body));
        },
        [](std::vector<std::string> &&varNames, ast::ExpressionPtr &&collection,
           ast::ExpressionPtr &&body, lexy::nullopt) -> ast::SelectionStatementPtr
        {
            return std::make_unique<ast::SelectionStatement>(
                std::move(varNames), std::move(collection), std::move(body));
        },
        [](std::vector<std::string> &&varNames, ast::ExpressionPtr &&collection,
           ast::ExpressionPtr &&body, ast::StatementPtr &&quant) -> ast::SelectionStatementPtr
        {
            return std::make_unique<ast::SelectionStatement>(
                std::move(varNames), std::move(collection), std::move(body), std::move(quant));
        });
};

struct Assignment
{
    static constexpr auto rule = dsl::p<Identifier> >>
                                 dsl::while_(dsl::ascii::space) >> dsl::lit_c<'='> >>
                                 dsl::while_(dsl::ascii::space) >> dsl::p<ExpressionProduct>;

    static constexpr auto value = lexy::callback<ast::StatementPtr>(
        [](std::string &&name, ast::ExpressionPtr &&expr) -> ast::StatementPtr
        {
            auto asg = std::make_unique<ast::AssignmentStatement>(std::move(name), std::move(expr));
            return std::make_unique<ast::Statement>(std::move(asg));
        });
};

struct ExceptQuantifier
{
    static constexpr auto whitespace = dsl::ascii::space | dsl::ascii::newline;
    static constexpr auto rule = LEXY_LIT("except") >> dsl::p<Quantifier>;
    static constexpr auto value = lexy::callback<ast::StatementPtr>(
        [](ast::StatementPtr &&inner) -> ast::StatementPtr
        {
            auto asg = std::make_unique<ast::ExceptStatement>(std::move(inner));
            return std::make_unique<ast::Statement>(std::move(asg));
        });
};

struct Statement
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto rule =
        dsl::p<Quantifier> | dsl::p<Assignment> | dsl::else_ >> dsl::p<ExceptQuantifier>;
    static constexpr auto value = lexy::forward<ast::StatementPtr>;
};

struct Block
{
    static constexpr auto rule = dsl::list(dsl::p<Statement>, dsl::sep(dsl::semicolon));

    static constexpr auto value =
        lexy::as_list<std::vector<ast::StatementPtr>> >>
        lexy::callback<ast::BlockPtr>([](std::vector<ast::StatementPtr> &&stmts) -> ast::BlockPtr
                                      { return std::make_unique<ast::Block>(std::move(stmts)); });
};

struct Description
{
    static constexpr auto rule = LEXY_LIT("description:") >>
                                 dsl::while_(dsl::ascii::space) >> dsl::p<String> >> LEXY_LIT(";");
    static constexpr auto value = lexy::as_string<std::string>;
};

struct Priority
{
    struct PriorityError
    {
        static constexpr auto rule = LEXY_LIT("Error");
        static constexpr auto value = lexy::constant(ast::Priority::ERROR);
    };

    struct PriorityInfo
    {
        static constexpr auto rule = LEXY_LIT("Info");
        static constexpr auto value = lexy::constant(ast::Priority::INFO);
    };

    struct PriorityWarn
    {
        static constexpr auto rule = LEXY_LIT("Warn");
        static constexpr auto value = lexy::constant(ast::Priority::WARN);
    };

    static constexpr auto rule = LEXY_LIT("priority:") >> dsl::while_(dsl::ascii::space) >>
                                 (dsl::p<PriorityError> | dsl::p<PriorityInfo> |
                                  dsl::p<PriorityWarn>) >>
                                 LEXY_LIT(";");
    static constexpr auto value = lexy::forward<ast::Priority>;
};

struct RuleDecl
{
    static constexpr auto whitespace = dsl::ascii::space | dsl::ascii::newline;
    static constexpr auto rule = LEXY_LIT("rule") >> dsl::p<Identifier> >>
                                 dsl::curly_bracketed(dsl::opt(dsl::p<Description>) +
                                                      dsl::opt(dsl::p<Priority>) + dsl::p<Block>);

    static constexpr auto value = lexy::callback<ast::Rule>(
        [](std::string &&name, std::string &&desc, lexy::nullopt,
           ast::BlockPtr &&block) -> ast::Rule {
            return ast::Rule(std::move(name), std::move(desc), ast::Priority::ERROR,
                             std::move(block));
        },
        [](std::string &&name, lexy::nullopt, ast::Priority prio,
           ast::BlockPtr &&block) -> ast::Rule
        { return ast::Rule(std::move(name), std::nullopt, prio, std::move(block)); },
        [](std::string &&name, std::string &&desc, ast::Priority prio,
           ast::BlockPtr &&block) -> ast::Rule
        { return ast::Rule(std::move(name), std::move(desc), prio, std::move(block)); },
        [](std::string &&name, lexy::nullopt, lexy::nullopt, ast::BlockPtr &&block) -> ast::Rule {
            return ast::Rule(std::move(name), std::nullopt, ast::Priority::ERROR, std::move(block));
        });
};

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

} // namespace lang::grammar