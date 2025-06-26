#pragma once

#include "../descriptors.hpp"
#include "../scope.hpp"

namespace dipp::details
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
            requires(std::copy_constructible<service_type>)
            : m_Value(value)
        {
        }

        constexpr base_injected(service_type& value) noexcept(
            std::is_nothrow_copy_constructible_v<service_type>)
            requires(((descriptor_type::lifetime == service_lifetime::singleton) ||
                      (descriptor_type::lifetime == service_lifetime::scope)))
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
}