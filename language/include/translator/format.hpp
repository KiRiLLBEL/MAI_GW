#pragma once

#include <string>

namespace cypher {
    using namespace std::literals::string_view_literals;
    static constexpr auto RuleNameFormat = "// [RULE]: {}"sv;
    static constexpr auto DescriptionFormat = "// [DESCRIPTION]: {}"sv;
    static constexpr auto PriorityFormat = "// [PRIORITY]: {}"sv;
    static constexpr auto InDevelopmentFormat = "// [NOW IN DEVELOPMENT]"sv;
    static constexpr auto WhereFormat = "WHERE {}"sv;
    static constexpr auto QuantFormat = "{}({} IN {} WHERE {})"sv;
    static constexpr auto MatchFormat = "MATCH (obj:{})"sv;
    static constexpr auto WithFormat = "WITH collect(obj) AS {}"sv;

    static constexpr auto SuperSetsContainsError = "Variable [{}] not in super set"sv;
    static constexpr auto SelectionFromRouteError = "Selection must be from route function"sv; 
}