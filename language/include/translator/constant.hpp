#pragma once
#include <string>
#include <unordered_set>

namespace cypher{
    using namespace std::literals::string_view_literals;
    static constexpr auto SystemSet = "system"sv;
    static constexpr auto ContainerSet = "container"sv;
    static constexpr auto ComponentSet = "component"sv;
    static constexpr auto CodeSet = "code"sv;
    static constexpr auto DeploySet = "deploy"sv;
    static constexpr auto InfrastructureSet = "infrastructure"sv;
    static std::unordered_set SuperSets {
        SystemSet,
        ContainerSet,
        ComponentSet,
        CodeSet,
        DeploySet,
        InfrastructureSet
    };

    static constexpr auto RouteFunc = "route"sv;
}