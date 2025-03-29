#pragma once

#include "ast/statement.hpp"
#include <array>
#include <ast/expression.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace lang::ast::cypher
{
using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;
static constexpr auto ruleNameFormat = "// [RULE]: {}"sv;
static constexpr auto descriptionFormat = "// [DESCRIPTION]: {}"sv;
static constexpr auto priorityFormat = "// [PRIORITY]: {}"sv;
static constexpr auto withAssignmentFormat = "WITH {} AS {}"sv;
static constexpr auto ternaryFormat = "CASE WHEN ({}) THEN ({}) ELSE ({}) END"sv;
static constexpr auto ifThenElseFormat = "CASE WHEN ({}) THEN ({}) ELSE ({}) END"sv;
static constexpr auto ifThenFormat = "CASE WHEN ({}) THEN ({}) ELSE (true) END"sv;

constexpr auto OperatorMap(const ExprType type)
{
    switch (type)
    {
    case ExprType::PLUS:
        return "{} + {}"s;
    case ExprType::MINUS:
        return "{} - {}"s;
    case ExprType::MULT:
        return "{} * {}"s;
    case ExprType::DIV:
        return "{} / {}"s;
    case ExprType::NEG:
        return "-{}"s;
    case ExprType::EQ:
        return "{} = {}"s;
    case ExprType::NOT_EQ:
        return "{} <> {}"s;
    case ExprType::LESS:
        return "{} < {}"s;
    case ExprType::GREATER:
        return "{} > {}"s;
    case ExprType::LESS_EQ:
        return "{} <= {}"s;
    case ExprType::GREATER_EQ:
        return "{} >= {}"s;
    case ExprType::IN:
        return "{} IN {}"s;
    case ExprType::AND:
        return "{} AND {}"s;
    case ExprType::OR:
        return "{} OR {}"s;
    case ExprType::XOR:
        return "{} XOR {}"s;
    case ExprType::ACCESS:
        return "{}.{}"s;
    case ExprType::SAFE_ACCESS:
        return "exists({}.{})"s;
    default:
        throw std::runtime_error{"undefined operator"};
    }
}

constexpr auto KeywordMap(const KeywordSets type)
{
    switch (type)
    {
    case KeywordSets::SYSTEM:
        return "SoftwareSystem"s;
    case KeywordSets::CONTAINER:
        return "Container"s;
    case KeywordSets::COMPONENT:
        return "Component"s;
    case KeywordSets::CODE:
        return "Code"s;
    case KeywordSets::DEPLOY:
        return "DeploymentNode"s;
    case KeywordSets::INFRASTRUCTURE:
        return "InfrastructureNode"s;
    case KeywordSets::NONE:
        return "[]"s;
    default:
        throw std::runtime_error{"undefined keyword"};
    }
}

constexpr auto QuantifierMap(const QuantifierType type)
{
    switch (type)
    {
    case QuantifierType::ALL:
        return "NOT EXISTS {{ {} NOT ({}) }}"s;
    case QuantifierType::ANY:
        return "EXISTS {{ {} ({}) }}"s;
    default:
        throw std::runtime_error{"undefined quantirier type"};
    }
}

constexpr auto QuantifierStartMap(const QuantifierType type)
{
    switch (type)
    {
    case QuantifierType::ALL:
        return "{} NOT ({})"s;
    case QuantifierType::ANY:
        return "{} ({})"s;
    default:
        throw std::runtime_error{"undefined quantirier type"};
    }
}

constexpr auto QuantifierExceptMap(const QuantifierType type)
{
    switch (type)
    {
    case QuantifierType::ALL:
        return "NOT ({})"s;
    case QuantifierType::ANY:
        return "({})"s;
    default:
        throw std::runtime_error{"undefined quantirier type"};
    }
}

static constexpr auto variableError = "Variable [{}] not exist in current context"sv;
static constexpr auto functionError = "Function [{}] not exist"sv;
} // namespace lang::ast::cypher