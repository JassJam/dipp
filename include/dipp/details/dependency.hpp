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
    auto get_tuple_from_scope_impl(ScopeTy& scope, std::index_sequence<Is...>)
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
        return get_tuple_from_scope_impl<DepsTy>(
            scope, std::make_index_sequence<std::tuple_size_v<dependencies_type>>{});
    }

    template<dependency_container_type DepsTy, typename ResultTy>
    bool has_error_in_tuple(const ResultTy& deps)
    {
        using dependencies_type = typename DepsTy::types;
        constexpr auto dependencies_size = std::tuple_size_v<std::decay_t<dependencies_type>>;

        return [&]<size_t... I>(std::index_sequence<I...>)
        {
            return ((std::get<I>(deps).has_error()) || ...);
        }(std::make_index_sequence<dependencies_size>{});
    }

#ifdef DIPP_USE_RESULT
    template<dependency_container_type DepsTy, typename ResultTy>
    auto get_error_from_tuple(ResultTy& deps) -> error_id
    {
        using dependencies_type = typename DepsTy::types;
        constexpr auto dependencies_size = std::tuple_size_v<std::decay_t<dependencies_type>>;

        return [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            error_id err_id{};
            ((void) (!err_id && (std::get<I>(deps).has_error() ? err_id = std::get<I>(deps).error()
                                                               : error_id{})),
             ...);
            return err_id;
        }(std::make_index_sequence<dependencies_size>{});
    }
#endif

    template<dependency_container_type DepsTy, typename ResultTy>
    auto unwrap_tuple_values(ResultTy&& deps)
    {
        using dependencies_type = typename DepsTy::types;
        constexpr auto dependencies_size = std::tuple_size_v<std::decay_t<dependencies_type>>;

        return [&]<std::size_t... I>(std::index_sequence<I...>) -> dependencies_type
        {
            return std::make_tuple(std::move(std::get<I>(deps).value())...);
        }(std::make_index_sequence<dependencies_size>{});
    }
}