#pragma once

#include "core.hpp"
#include "move_only_any.hpp"

namespace dipp
{
    template<class From, class To>
    concept convertible_to =
        std::is_convertible_v<From, To> && requires { static_cast<To>(std::declval<From>()); };

    //

    template<typename Ty>
    concept service_policy_service_map_type = requires(Ty t) {
        t.find(std::declval<type_key_pair>());
        t.emplace(std::declval<type_key_pair>(), std::declval<typename Ty::service_info>());
    };

    template<typename Ty>
    concept service_policy_instance_map_type =
        requires(Ty t) { t.find(std::declval<type_key_pair>()); };

    template<typename Ty>
    concept service_policy_type = requires(Ty t) {
        typename Ty::service_info;
        service_policy_service_map_type<typename Ty::service_map_type>;
    };

    //

    template<typename Ty>
    concept service_provider_type = requires {
        typename Ty::singleton_memory_type;
        typename Ty::storage_type;
        typename Ty::scope_type;
        typename Ty::collection_type;

        { std::declval<Ty>().create_scope() } -> std::same_as<typename Ty::scope_type>;
        { std::declval<Ty>().root_scope() } -> std::same_as<typename Ty::scope_type&>;
    };

    template<typename Ty>
    concept service_scope_type = requires(Ty t) {
        typename Ty::storage_type;
        typename Ty::singleton_storage_type;
        typename Ty::scoped_storage_type;
    };

    template<typename Ty>
    concept service_descriptor_type = requires(Ty t) {
        typename Ty::value_type;
        typename Ty::service_type;

        { Ty::lifetime } -> convertible_to<service_lifetime>;
        { t.load(std::declval<typename Ty::scope_type&>()) } -> std::same_as<move_only_any>;
    };

    template<typename Ty>
    concept service_storage_memory_type = requires(Ty t) {
        typename Ty::instance_info;
        service_policy_instance_map_type<typename Ty::instance_map_type>;
    };

    //

    template<typename Ty>
    concept base_injected_type = requires(Ty t) {
        typename Ty::descriptor_type;
        typename Ty::value_type;

        typename Ty::reference_type;
        typename Ty::const_reference_type;
        typename Ty::pointer_type;
        typename Ty::const_pointer_type;

        { std::declval<Ty>().get() } -> std::same_as<typename Ty::reference_type>;
        { std::declval<const Ty>().get() } -> std::same_as<typename Ty::const_reference_type>;
        { std::declval<Ty>().operator->() } -> std::same_as<typename Ty::pointer_type>;
        { std::declval<const Ty>().operator->() } -> std::same_as<typename Ty::const_pointer_type>;
    };

    //

    template<typename Ty>
    concept dependency_container_type = requires(Ty t) { typename Ty::types; };
} // namespace dipp