#pragma once

#include "concepts.hpp"

namespace dipp::details
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

        return std::make_tuple(scope.template get<std::tuple_element_t<Is, dependencies>>()...);
    }

    /// <summary>
    /// Get a tuple of dependencies from a scope.
    /// </summary>
    template<dependency_container_type DepsTy, service_scope_type ScopeTy>
    auto get_tuple_from_scope(ScopeTy& scope) //-> result<typename DepsTy::types>
    {
        using dependencies_type = typename DepsTy::types;
        constexpr auto dependencies_size = std::tuple_size_v<std::decay_t<dependencies_type>>;

        // load all dependencies from the scope
        auto dependencies = get_tuple_from_scope<DepsTy>(
            scope, std::make_index_sequence<std::tuple_size_v<dependencies_type>>{});

#ifdef DIPP_USE_RESULT
        // Check if any results have errors
        error_id first_error;
        bool has_error = [&]<size_t... I>(std::index_sequence<I...>)
        {
            return ((std::get<I>(dependencies).has_error() &&
                     (first_error = std::get<I>(dependencies).error(), true)) ||
                    ...);
        }(std::make_index_sequence<dependencies_size>{});

        // If any errors, return the first error
        // we found
        if (has_error)
        {
            return result<dependencies_type>{first_error};
        }
#endif

        // All results are valid, combine values with move
        return [&]<std::size_t... I>(std::index_sequence<I...>) -> result<dependencies_type>
        {
            return std::make_tuple(std::move(std::get<I>(dependencies).value())...);
        }(std::make_index_sequence<dependencies_size>{});
    }
}