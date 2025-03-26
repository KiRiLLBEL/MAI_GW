#pragma once
#include <ast/ast.hpp>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace lang::ast::json
{
template <typename T> class Serializer
{
public:
    nlohmann::json operator()(const T & /*value*/) const
    {
        return nlohmann::json{{"Unimplemented", "No serialization for this type"}};
    }
};

template <typename... Ts> class Serializer<std::variant<Ts...>>
{
public:
    nlohmann::json operator()(const std::variant<Ts...> &var) const
    {
        return std::visit([&](auto &subValue) -> nlohmann::json
                          { return Serializer<std::decay_t<decltype(subValue)>>{}(subValue); },
                          var);
    }
};

template <typename T> class Serializer<std::unique_ptr<T>>
{
public:
    nlohmann::json operator()(const std::unique_ptr<T> &ptr) const
    {
        if (!ptr)
        {
            throw std::runtime_error{"broken AST: ptr is null"};
        }

        return Serializer<T>{}(*ptr);
    }
};

template <> class Serializer<Priority>
{
public:
    nlohmann::json operator()(const Priority &priority) const
    {
        return nlohmann::json{magic_enum::enum_name(priority)};
    }
};

template <KeywordSets K> class Serializer<KeywordExpr<K>>
{
public:
    nlohmann::json operator()(const KeywordExpr<K> & /*unused*/) const
    {
        nlohmann::json jOut;
        jOut["keyword"] = magic_enum::enum_name(K);
        return jOut;
    }
};

template <typename T> using DecayT = typename std::decay<T>::type;

template <typename U> nlohmann::json Serialize(U &&value)
{
    using CleanType = DecayT<U>;
    return Serializer<CleanType>{}(std::forward<U>(value));
}

} // namespace lang::ast::json
