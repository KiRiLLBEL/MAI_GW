#pragma once

#include "statement.hpp"

#include <memory>
#include <string>

namespace lang::ast
{

enum class Priority
{
    INFO,
    WARN,
    ERROR
};

struct Block
{
    BodyStatementList statements;
};

using BlockPtr = std::unique_ptr<Block>;

struct Rule
{
    std::string name;
    std::string description;
    Priority priority{Priority::ERROR};
    BlockPtr calls;
};

} // namespace lang::ast