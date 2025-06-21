#pragma once

#include "base.hpp"

namespace dipp::details
{
    template<typename Ty,
             service_lifetime Lifetime,
             dependency_container_type DepsTy = dependency<>,
             size_t Key = size_t{},
             service_scope_type ScopeTy = service_scope>
    struct injected_shared
        : public base_injected<shared_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>
    {
    public:
        using base_type =
            base_injected<shared_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;
        using value_type = typename base_type::value_type;

    public:
        using base_type::base_type;

        using base_type::detach;
        using base_type::get;
        using base_type::ptr;
        using base_type::operator->;
        using base_type::operator*;

    public:
        constexpr operator const value_type&() const noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return get();
        }
        constexpr operator value_type&() noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return get();
        }

        constexpr operator value_type() && noexcept
            requires(Lifetime == service_lifetime::transient)
        {
            return detach();
        }

        constexpr operator const value_type*() const noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return std::addressof(static_cast<const value_type&>(*this));
        }
        constexpr operator value_type*() noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return std::addressof(static_cast<value_type&>(*this));
        }

        constexpr operator const Ty&() const noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return *static_cast<const value_type&>(*this);
        }
        constexpr operator Ty&() noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return *static_cast<value_type&>(*this);
        }

        constexpr operator const Ty*() const noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return std::addressof(static_cast<const Ty&>(*this));
        }
        constexpr operator Ty*() noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return std::addressof(static_cast<Ty&>(*this));
        }
    };
}