#pragma once
#include <string>

namespace cypher{
    using namespace std::literals::string_view_literals;
    static constexpr auto SystemSet = "system"sv;
    static constexpr auto ContainerSet = "container"sv;
    static constexpr auto ComponentSet = "component"sv;
    static constexpr auto CodeSet = "code"sv;
    static constexpr auto DeploySet = "deploy"sv;
    static constexpr auto InfrastructureSet = "infrastructure"sv;
}