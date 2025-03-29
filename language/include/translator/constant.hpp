#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace lang::ast::cypher
{
using namespace std::string_literals;

static const auto routeFunction = "({})-[*1..]->({})"s;
static const auto crossFunction = "[ x IN {} WHERE x IN {} ]"s;
static const auto unionFunction =
    "WITH {} + {} AS combined UNWIND combined AS item RETURN collect(DISTINCT item) AS unionSet"s;

static const std::unordered_map<std::string, std::string> functionMap{
    {"route", routeFunction}, {"cross", crossFunction}, {"union", unionFunction}};
} // namespace lang::ast::cypher