#pragma once

#include "constant.hpp"
#include "context.hpp"
#include "fmt/base.h"
#include "fmt/format.h"
#include "translator/translator.hpp"
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <translator/format.hpp>

#include <algorithm>
#include <ast/expression.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

namespace lang::ast::cypher
{

template <typename... Args>
auto ErrorHelper(std::string_view error, Args... args) -> std::runtime_error
{
    return std::runtime_error{fmt::format(fmt::runtime(error), args...)};
}

auto SetsMapping(KeywordSets set)
{
    switch (set)
    {
    case KeywordSets::SYSTEM:
        return KeywordSets::CONTAINER;
    case KeywordSets::CONTAINER:
        return KeywordSets::COMPONENT;
    case KeywordSets::COMPONENT:
        return KeywordSets::CODE;
    case KeywordSets::DEPLOY:
    case KeywordSets::INFRASTRUCTURE:
    case KeywordSets::NONE:
    case KeywordSets::CODE:
    default:
        throw std::runtime_error{"No this mapping"};
    }
}

}; // namespace lang::ast::cypher