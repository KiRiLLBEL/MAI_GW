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

}; // namespace lang::ast::cypher