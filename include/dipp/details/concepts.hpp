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
        // Required types
        typename Ty::iterator;
        typename Ty::const_iterator;

        // Required functions
        // iterator find(const type_key_pair&);
        t.find(std::declval<type_key_pair>());

        // void erase(iterator);
        t.erase(std::declval<typename Ty::iterator>());

        // iterator end();
        t.end();

        // instance_info& emplace(const type_key_pair&, descriptor_type&, scope_type&);
    };

    template<typename Ty>
    concept service_policy_type = requires(Ty t) {
        // Required types
        service_policy_service_map_type<typename Ty::service_map_type>;
    };

    //

    template<typename Ty>
    concept service_policy_instance_map_type = requires(Ty t) {
        // Required functions
        // instance_info* find(const type_key_pair&);
        t.find(std::declval<type_key_pair>());

        // instance_info& emplace(const type_key_pair&, descriptor_type&, scope_type&);
    };

    template<typename Ty>
    concept service_storage_memory_type = requires(Ty t) {
        // Required types
        service_policy_instance_map_type<typename Ty::instance_map_type>;
    };

    //

    template<typename Ty>
    concept service_provider_type = requires {
        // Required types
        typename Ty::singleton_memory_type;
        typename Ty::storage_type;
        typename Ty::scope_type;
        typename Ty::collection_type;

        // Required functions
        // scope_type create_scope();
        { std::declval<Ty>().create_scope() } -> std::same_as<typename Ty::scope_type>;

        // scope_type& root_scope();
        { std::declval<Ty>().root_scope() } -> std::same_as<typename Ty::scope_type&>;
    };

    template<typename Ty>
    concept service_scope_type = requires(Ty t) {
        // Required types
        typename Ty::storage_type;
        typename Ty::singleton_storage_type;
        typename Ty::scoped_storage_type;
    };

    template<typename Ty>
    concept service_descriptor_type = requires(Ty t) {
        // Required types
        typename Ty::value_type;
        typename Ty::service_type;
        { Ty::lifetime } -> convertible_to<service_lifetime>;

        // Required functions
        // move_only_any load(scope_type& scope) const;
        { t.load(std::declval<typename Ty::scope_type&>()) } -> std::same_as<move_only_any>;
    };

    //

    template<typename Ty>
    concept base_injected_type = requires(Ty t) {
        // Required types
        typename Ty::descriptor_type;
        typename Ty::value_type;

        typename Ty::reference_type;
        typename Ty::const_reference_type;
        typename Ty::pointer_type;
        typename Ty::const_pointer_type;

        // Required functions
        // reference_type get() const;
        { std::declval<Ty>().get() } -> std::same_as<typename Ty::reference_type>;

        // const_reference_type get() const;
        { std::declval<const Ty>().get() } -> std::same_as<typename Ty::const_reference_type>;

        // pointer_type operator->();
        { std::declval<Ty>().operator->() } -> std::same_as<typename Ty::pointer_type>;

        // const_pointer_type operator->() const;
        { std::declval<const Ty>().operator->() } -> std::same_as<typename Ty::const_pointer_type>;
    };

    //

    template<typename Ty>
    concept dependency_container_type = requires(Ty t) {
        // Required types
        typename Ty::types;
    };
} // namespace dipp