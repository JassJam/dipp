#pragma once

#include "descriptor.hpp"
#include "scope.hpp"

namespace dipp
{
    template<service_descriptor_type DescTy, size_t Key>
    class base_injected
    {
    public:
        using descriptor_type = DescTy;
        using value_type = typename descriptor_type::value_type;
        using service_type = typename descriptor_type::service_type;
        static constexpr size_t key = Key;

        using reference_type = std::add_lvalue_reference_t<std::remove_cvref_t<value_type>>;
        using const_reference_type =
            std::add_lvalue_reference_t<std::add_const_t<std::remove_cvref_t<value_type>>>;
        using pointer_type = std::add_pointer_t<std::remove_cvref_t<value_type>>;
        using const_pointer_type =
            std::add_pointer_t<std::add_const_t<std::remove_cvref_t<value_type>>>;

    public:
        constexpr base_injected(service_type&& value) noexcept(
            std::is_nothrow_move_constructible_v<value_type>)
            : m_Value(std::move(value))
        {
        }

        constexpr base_injected(const service_type& value) noexcept(
            std::is_nothrow_copy_constructible_v<service_type>)
            : m_Value(value)
        {
        }

        constexpr base_injected(service_type& value) noexcept(
            std::is_nothrow_copy_constructible_v<service_type>)
            : m_Value(value)
        {
        }

        template<typename... Args>
        constexpr base_injected(Args&&... args) noexcept(
            std::is_nothrow_constructible_v<service_type, Args...>)
            requires(std::constructible_from<service_type, Args...>)
            : m_Value(std::forward<Args>(args)...)
        {
        }

    public:
        /// <summary>
        /// Detach the service from the injected object.
        /// </summary>
        template<
            typename = std::enable_if_t<descriptor_type::lifetime == service_lifetime::transient>>
        [[nodiscard]] constexpr auto detach() noexcept
        {
            return std::move(m_Value);
        }

        /// <summary>
        /// Get the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr const_reference_type get() const noexcept
        {
            return m_Value;
        }

        /// <summary>
        /// Get the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr reference_type get() noexcept
        {
            return m_Value;
        }

        /// <summary>
        /// Get the address of the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr const_pointer_type ptr() const noexcept
        {
            return std::addressof(get());
        }

        /// <summary>
        /// Get the address of the service from the injected object.
        /// </summary>
        /// <returns></returns>
        [[nodiscard]] constexpr pointer_type ptr() noexcept
        {
            return std::addressof(get());
        }

        /// <summary>
        /// Get the address of the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr const_pointer_type operator->() const noexcept
        {
            return ptr();
        }

        /// <summary>
        /// Get the address of the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr pointer_type operator->() noexcept
        {
            return ptr();
        }

        /// <summary>
        /// Get the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr const_reference_type operator*() const noexcept
        {
            return get();
        }

        /// <summary>
        /// Get the service from the injected object.
        /// </summary>
        [[nodiscard]] constexpr reference_type operator*() noexcept
        {
            return get();
        }

    private:
        service_type m_Value;
    };

    //

    template<typename Ty,
             service_lifetime Lifetime,
             dependency_container_type DepsTy = dependency<>,
             size_t Key = size_t{},
             service_scope_type ScopeTy = default_service_scope>
    struct injected_functor
        : public base_injected<functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>
    {
    public:
        using base_type =
            base_injected<functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;

        using value_type = typename base_type::value_type;

    public:
        using base_type::base_type;

        using base_type::detach;
        using base_type::get;
        using base_type::ptr;
        using base_type::operator->;
        using base_type::operator*;

    public:
        constexpr operator const Ty&() const noexcept
        {
            return get();
        }
        constexpr operator Ty&() noexcept
        {
            return get();
        }

        constexpr operator const Ty*() const noexcept
        {
            return std::addressof(static_cast<const Ty&>(*this));
        }
        constexpr operator Ty*() noexcept
        {
            return std::addressof(static_cast<Ty&>(*this));
        }
    };

    template<typename Ty,
             service_lifetime Lifetime,
             dependency_container_type DepsTy = dependency<>,
             size_t Key = size_t{},
             service_scope_type ScopeTy = default_service_scope>
    struct injected_unique
        : public base_injected<unique_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>
    {
    public:
        using base_type =
            base_injected<unique_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;
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

    template<typename Ty,
             service_lifetime Lifetime,
             dependency_container_type DepsTy = dependency<>,
             size_t Key = size_t{},
             service_scope_type ScopeTy = default_service_scope>
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

    template<typename Ty,
             service_lifetime Lifetime,
             dependency_container_type DepsTy = dependency<>,
             size_t Key = size_t{},
             service_scope_type ScopeTy = default_service_scope>
        requires(Lifetime != service_lifetime::transient)
    struct injected_ref
        : public base_injected<
              local_service_descriptor<std::reference_wrapper<Ty>, Lifetime, ScopeTy, DepsTy>,
              Key>
    {
    public:
        using base_type = base_injected<
            local_service_descriptor<std::reference_wrapper<Ty>, Lifetime, ScopeTy, DepsTy>,
            Key>;
        using value_type = typename base_type::value_type;

    public:
        using base_type::base_type;

        using base_type::detach;
        using base_type::get;
        using base_type::ptr;
        using base_type::operator->;
        using base_type::operator*;

    public:
        constexpr operator const Ty&() const noexcept
        {
            return get();
        }
        constexpr operator Ty&() noexcept
        {
            return get();
        }

        constexpr operator const Ty*() const noexcept
        {
            return std::addressof(static_cast<const Ty&>(*this));
        }
        constexpr operator Ty*() noexcept
        {
            return std::addressof(static_cast<Ty&>(*this));
        }
    };

    template<typename Ty,
             service_lifetime Lifetime,
             dependency_container_type DepsTy = dependency<>,
             size_t Key = size_t{},
             service_scope_type ScopeTy = default_service_scope>
    struct injected
        : public base_injected<local_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>
    {
    public:
        using base_type =
            base_injected<local_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>, Key>;
        using value_type = typename base_type::value_type;

    public:
        using base_type::base_type;

        using base_type::detach;
        using base_type::get;
        using base_type::ptr;
        using base_type::operator->;
        using base_type::operator*;

    public:
        constexpr operator const Ty&() const noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return get();
        }
        constexpr operator Ty&() noexcept
            requires(Lifetime != service_lifetime::transient)
        {
            return get();
        }

        constexpr operator Ty() && noexcept
            requires(Lifetime == service_lifetime::transient)
        {
            return detach();
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
} // namespace dipp