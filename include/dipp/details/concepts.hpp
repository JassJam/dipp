#pragma once

#include "core.hpp"

namespace dipp
{
    template<class From, class To>
    concept convertible_to = std::is_convertible_v<From, To> && requires { static_cast<To>(std::declval<From>()); };

    template<typename Ty>
    concept service_policy_service_map_type = requires(Ty t) {
        typename Ty::service_key_type;
        typename Ty::service_info;

        {
            Ty::make_key(std::declval<size_t>(), std::declval<size_t>())
        } -> convertible_to<typename Ty::service_key_type>;

        t.find(std::declval<typename Ty::service_key_type>());
        t.emplace(std::declval<typename Ty::service_key_type>(), std::declval<typename Ty::service_info>());
    };

    template<typename Ty>
    concept service_policy_instance_map_type = requires(Ty t) {
        typename Ty::instance_key_type;
        typename Ty::instance_info;

        {
            Ty::make_key(std::declval<size_t>(), std::declval<size_t>())
        } -> convertible_to<typename Ty::instance_key_type>;

        t.find(std::declval<typename Ty::instance_key_type>());
        t.emplace(std::declval<typename Ty::instance_key_type>(), std::declval<typename Ty::instance_info>());
    };

    template<typename Ty>
    concept service_policy_type = requires(Ty t) {
        typename Ty::service_info;
        typename Ty::service_key_type;
        service_policy_service_map_type<typename Ty::service_map_type>;
    };

    template<typename Ty>
    concept instance_policy_type = requires(Ty t) {
        typename Ty::instance_info;
        typename Ty::instance_key_type;
        service_policy_instance_map_type<typename Ty::instance_map_type>;
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
        { t.load(std::declval<typename Ty::scope_type&>()) } -> std::same_as<typename Ty::value_type>;
    };

    template<typename Ty>
    concept service_storage_memory_type = requires(Ty t) {
        service_policy_type<typename Ty::policy_type>;
        {
            t.find(std::declval<typename Ty::policy_type::instance_key_type>())
        } -> std::same_as<typename Ty::policy_type::instance_info*>;
        {
            t.emplace(std::declval<typename Ty::policy_type::instance_key_type>(),
                      std::declval<typename Ty::policy_type::instance_info>())
        } -> std::same_as<typename Ty::policy_type::instance_info*>;
    };

    //

    template<typename Ty>
    concept base_injected_type = requires(Ty t) {
        typename Ty::descriptor_type;
        typename Ty::value_type;
        typename Ty::string_hash_type;

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