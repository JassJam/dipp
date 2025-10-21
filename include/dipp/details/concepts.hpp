#pragma once

#include "service_lifetime.hpp"
#include "type_key_pair.hpp"
#include "move_only_any.hpp"

namespace dipp::details
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
    };

    template<typename Ty>
    concept service_policy_type = requires {
        // Required types
        typename Ty::service_map_type;
        service_policy_service_map_type<typename Ty::service_map_type>;
    };

    //

    template<typename Ty>
    concept service_policy_instance_map_type = requires(Ty t) {
        // Required functions
        // instance_info* find(const type_key_pair&);
        t.find(std::declval<type_key_pair>());
    };

    template<typename Ty>
    concept service_storage_memory_type = requires {
        // Required types
        typename Ty::instance_map_type;
        service_policy_instance_map_type<typename Ty::instance_map_type>;
    };

    //

    template<typename Ty>
    concept service_provider_type = requires(Ty t) {
        // Required types
        typename Ty::scoped_storage_type;
        typename Ty::singleton_storage_type;
        typename Ty::storage_type;
        typename Ty::scope_type;
        typename Ty::collection_type;

        // Required functions
        // scope_type create_scope();
        { t.create_scope() } -> std::same_as<typename Ty::scope_type>;

        // scope_type& root_scope();
        { t.root_scope() } -> std::same_as<typename Ty::scope_type&>;
    };

    template<typename Ty>
    concept service_scope_type = requires {
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
        { t.get() } -> std::same_as<typename Ty::reference_type>;

        // const_reference_type get() const;
        { std::as_const(t).get() } -> std::same_as<typename Ty::const_reference_type>;

        // pointer_type operator->();
        { t.operator->() } -> std::same_as<typename Ty::pointer_type>;

        // const_pointer_type operator->() const;
        { std::as_const(t).operator->() } -> std::same_as<typename Ty::const_pointer_type>;
    };

    //

    template<typename Ty>
    concept dependency_container_type = requires {
        // Required types
        typename Ty::types;
    };

    //

    template<typename Ty>
    concept container_type = requires(Ty t) {
        // Required functions
        // void reserve(size_t);
        t.reserve(std::declval<size_t>());

        // has emplace_back(Args&&...);
        t.emplace_back(std::declval<typename Ty::value_type>());

        // has move contructor
        requires std::is_move_constructible_v<Ty>;

        // has default constructor
        requires std::is_default_constructible_v<Ty>;
    };
}