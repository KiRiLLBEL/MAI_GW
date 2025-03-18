#pragma once
#include <set>
#include <string>
#include <unordered_set>

namespace cypher
{
using namespace std::literals::string_view_literals;
static constexpr auto systemSet = "system"sv;
static constexpr auto containerSet = "container"sv;
static constexpr auto componentSet = "component"sv;
static constexpr auto codeSet = "code"sv;
static constexpr auto deploySet = "deploy"sv;
static constexpr auto infrastructureSet = "infrastructure"sv;
static const std::unordered_set superSets{systemSet, containerSet, componentSet,
                                          codeSet,   deploySet,    infrastructureSet};

static constexpr auto routeFunc = "route"sv;
} // namespace cypher