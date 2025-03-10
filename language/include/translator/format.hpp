#pragma once

#include <string>

namespace cypher {
    using namespace std::literals::string_view_literals;
    static constexpr auto RuleNameFormat = "// [RULE]: {}"sv;
    static constexpr auto DescriptionFormat = "// [DESCRIPTION]: {}"sv;
    static constexpr auto PriorityFormat = "// [PRIORITY]: {}"sv;
    static constexpr auto InDevelopmentFormat = "// [NOW IN DEVELOPMENT]"sv;
}