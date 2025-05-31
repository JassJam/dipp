#pragma once

#ifdef DIPP_USE_RESULT
    #include <boost/leaf.hpp>

namespace dipp
{
    template<typename Ty>
    using result = boost::leaf::result<Ty>;

    template<typename Ty, typename... Args>
    inline result<Ty> make_result(Args&&... args)
    {
        return boost::leaf::make_result<Ty>(std::forward<Args>(args)...);
    }

    template<typename Error>
    inline auto make_error(const Error& error)
    {
        return boost::leaf::new_error(error);
    }
} // namespace dipp

#else

namespace dipp
{
    template<typename Ty>
    struct result
    {
    public:
        constexpr explicit result(Ty&& value) noexcept(
            std::is_nothrow_move_constructible<Ty>::value)
            : m_Value(std::forward<Ty>(value))
        {
        }

        constexpr explicit result(const Ty& value) noexcept(
            std::is_nothrow_copy_constructible<Ty>::value)
            : m_Value(value)
        {
        }

        template<typename... Args>
        constexpr explicit result(Args&&... args) noexcept(
            std::is_nothrow_constructible<Ty, Args...>::value)
            : m_Value(std::forward<Args>(args)...)
        {
        }

        constexpr result(const result& other) = default;
        constexpr result(result&& other) = default;

        constexpr result& operator=(const result& other) = default;
        constexpr result& operator=(result&& other) = default;

        constexpr result& operator=(Ty&& value) noexcept(std::is_nothrow_move_assignable<Ty>::value)
        {
            m_Value = std::forward<Ty>(value);
            return *this;
        }
        constexpr result& operator=(const Ty& value) noexcept(
            std::is_nothrow_copy_assignable<Ty>::value)
        {
            m_Value = value;
            return *this;
        }

        constexpr ~result() = default;

        [[nodiscard]]
        constexpr operator Ty&() &
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr operator const Ty&() const&
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr operator Ty&&() &&
        {
            return std::move(m_Value);
        }

        [[nodiscard]]
        constexpr Ty& value()
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr const Ty& value() const
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr bool has_value() const
        {
            return true; // Always has value in this implementation
        }

        [[nodiscard]]
        constexpr bool operator!() const
        {
            return false; // Always valid in this implementation
        }

        [[nodiscard]]
        constexpr bool has_error() const
        {
            return false; // No error handling in this implementation
        }

    private:
        Ty m_Value;
    };

    template<typename Ty, typename... Args>
    constexpr inline result<Ty> make_result(Args&&... args)
    {
        return result<Ty>(std::forward<Args>(args)...);
    }

    template<typename Error>
        requires std::is_base_of_v<std::exception, Error>
    [[nodiscard]] constexpr inline auto make_error(const Error& error)
    {
        throw error;
    }
} // namespace dipp
#endif
