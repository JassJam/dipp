#pragma once

#include "descriptor.hpp"
#include "scope.hpp"

namespace dipp
{
    template<service_descriptor_type DescTy, string_literal Key = string_literal<0>{}> class base_injected
    {
    public:
        using descriptor_type               = DescTy;
        using value_type                    = typename descriptor_type::value_type;
        using service_type                  = typename descriptor_type::service_type;
        using string_literal_type           = decltype(Key);
        static constexpr string_literal key = Key;

        using reference_type       = std::add_lvalue_reference_t<std::remove_cvref_t<value_type>>;
        using const_reference_type = std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<value_type>>>;
        using pointer_type         = std::add_pointer_t<std::remove_cvref_t<value_type>>;
        using const_pointer_type   = std::add_pointer_t<std::add_const_t<std::remove_cvref_t<value_type>>>;

        constexpr base_injected(service_type&& value) noexcept(std::is_nothrow_move_constructible_v<service_type>) :
            m_Value(std::move(value))
        {
        }

        constexpr base_injected(const service_type& value) noexcept(
            std::is_nothrow_copy_constructible_v<service_type>) : m_Value(value)
        {
        }

        constexpr base_injected(service_type& value) noexcept(std::is_nothrow_copy_constructible_v<service_type>) :
            m_Value(value)
        {
        }

    public:
        [[nodiscard]] constexpr auto detach() noexcept
        {
            return std::move(m_Value);
        }

        [[nodiscard]] constexpr const_reference_type get() const noexcept
        {
            return m_Value;
        }

        [[nodiscard]] constexpr reference_type get() noexcept
        {
            return m_Value;
        }

        [[nodiscard]] constexpr const_pointer_type ptr() const noexcept
        {
            return std::addressof(get());
        }

        [[nodiscard]] constexpr pointer_type ptr() noexcept
        {
            return std::addressof(get());
        }

        [[nodiscard]] constexpr const_pointer_type operator->() const noexcept
        {
            return ptr();
        }

        [[nodiscard]] constexpr pointer_type operator->() noexcept
        {
            return ptr();
        }

        [[nodiscard]] constexpr const_reference_type operator*() const noexcept
        {
            return get();
        }

        [[nodiscard]] constexpr reference_type operator*() noexcept
        {
            return get();
        }

    private:
        service_type m_Value;
    };

    //

    // clang-format off
    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy = dependency<>, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected_functor = base_injected<functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;

    template<typename Ty, service_lifetime Lifetime, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected_extern = base_injected<extern_service_descriptor<Ty, Lifetime, ScopeTy>, Key>;

    template<typename Ty, service_lifetime Lifetime, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected_const_extern = base_injected<const_extern_service_descriptor<Ty, Lifetime, ScopeTy>, Key>;

    template<typename Ty, service_lifetime Lifetime, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected_extern_shared = base_injected<extern_shared_service_descriptor<Ty, Lifetime, ScopeTy>, Key>;

    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy = dependency<>, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected_unique = base_injected<unique_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;

    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy = dependency<>, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected_shared = base_injected<shared_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;

    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy = dependency<>, string_literal Key = string_literal<0>{}, service_scope_type ScopeTy = default_service_scope>
    using injected = base_injected<local_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;
    // clang-format on
} // namespace dipp