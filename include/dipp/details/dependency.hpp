#pragma once

#include "concepts.hpp"

namespace dipp
{
    template<base_injected_type... Deps>
    struct dependency
    {
        using types = std::tuple<Deps...>;
    };

    template<>
    struct dependency<>
    {
        using types = std::tuple<>;
    };

    /// <summary>
    /// Get a tuple of dependencies from a scope.
    /// </summary>
    template<dependency_container_type DepsTy, service_scope_type ScopeTy, std::size_t... Is>
    auto get_tuple_from_scope(ScopeTy& scope, std::index_sequence<Is...>)
    {
        using dependencies = typename DepsTy::types;

        return std::make_tuple(
            scope.template get<std::tuple_element_t<Is, dependencies>>().value()...);
    }

    /// <summary>
    /// Get a tuple of dependencies from a scope.
    /// </summary>
    template<dependency_container_type DepsTy, service_scope_type ScopeTy>
    auto get_tuple_from_scope(ScopeTy& scope)
    {
        using dependencies = typename DepsTy::types;

        return get_tuple_from_scope<DepsTy>(
            scope, std::make_index_sequence<std::tuple_size_v<dependencies>>{});
    }
} // namespace dipp