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
    template<service_scope_type ScopeTy, dependency_container_type DepsTy, std::size_t... Is>
    auto get_tuple_from_scope(ScopeTy& scope, std::index_sequence<Is...>)
    {
        using dependencies = typename DepsTy::types;

        return std::tuple<std::invoke_result_t<
            decltype(&ScopeTy::template get<std::tuple_element_t<Is, dependencies>>),
            ScopeTy>...>{scope.template get<std::tuple_element_t<Is, dependencies>>()...};
    }

    /// <summary>
    /// Get a tuple of dependencies from a scope.
    /// </summary>
    template<service_scope_type ScopeTy, dependency_container_type DepsTy>
    auto get_tuple_from_scope(ScopeTy& scope)
    {
        return get_tuple_from_scope<ScopeTy, DepsTy>(
            scope, std::make_index_sequence<std::tuple_size_v<typename DepsTy::types>>{});
    }
} // namespace dipp