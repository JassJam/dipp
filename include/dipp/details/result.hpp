#pragma once

#include <type_traits>

#ifdef DIPP_USE_RESULT
    #include <boost/leaf.hpp>

namespace dipp::details
{
    template<typename Ty>
    using result = boost::leaf::result<Ty>;

    using error_id = boost::leaf::error_id;

    template<typename Ty, typename... Args>
    inline result<Ty> make_result(Args&&... args)
    {
        return result<Ty>(std::forward<Args>(args)...);
    }

    template<typename Error>
    inline auto make_error(const Error& error)
    {
        return boost::leaf::new_error(error);
    }
}

    #define DIPP_RETURN_ERROR(x) return dipp::details::make_error(x)

#else

namespace dipp::details
{
    template<typename Ty>
    struct result
    {
    public:
        constexpr result(Ty&& value) noexcept(std::is_nothrow_move_constructible<Ty>::value)
            : m_Value(std::forward<Ty>(value))
        {
        }

        constexpr result(const Ty& value) noexcept(std::is_nothrow_copy_constructible<Ty>::value)
            : m_Value(value)
        {
        }

        template<typename... Args>
        constexpr result(Args&&... args) noexcept(std::is_nothrow_constructible<Ty, Args...>::value)
            : m_Value(std::forward<Args>(args)...)
        {
        }

        constexpr result(const result& other) = default;
        constexpr result(result&& other) = default;

        constexpr result& operator=(const result& other) = default;
        constexpr result& operator=(result&& other) = default;

        constexpr ~result() = default;

    public:
        [[nodiscard]]
        constexpr Ty& operator*() &
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr Ty&& operator*() &&
        {
            return std::move(m_Value);
        }

        [[nodiscard]]
        constexpr const Ty& operator*() const&
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr const Ty&& operator*() const&&
        {
            return std::move(m_Value);
        }

    public:
        [[nodiscard]]
        constexpr operator Ty&() &
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr operator Ty&&() &&
        {
            return std::move(m_Value);
        }

        [[nodiscard]]
        constexpr operator const Ty&() const&
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr operator const Ty&&() const&&
        {
            return std::move(m_Value);
        }

    public:
        [[nodiscard]]
        constexpr Ty& value() &
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr Ty&& value() &&
        {
            return std::move(m_Value);
        }

        [[nodiscard]]
        constexpr const Ty& value() const&
        {
            return m_Value;
        }

        [[nodiscard]]
        constexpr const Ty&& value() const&&
        {
            return std::move(m_Value);
        }

    public:
        [[nodiscard]]
        constexpr bool has_value() const
        {
            return true;
        }

        explicit operator bool() const noexcept
        {
            return has_value();
        }

        [[nodiscard]]
        constexpr bool has_error() const
        {
            return false;
        }

        constexpr void error() const
        {
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
    [[nodiscard]] constexpr inline void make_error(const Error& error)
    {
        throw error;
    }
}

    #define DIPP_RETURN_ERROR(x) dipp::details::make_error(x)

#endif

namespace dipp::details
{
    template<typename Ty>
    struct is_result_type : std::false_type
    {
    };

    template<typename Ty>
    struct is_result_type<result<Ty>> : std::true_type
    {
    };

    template<typename Ty>
    inline constexpr bool is_result_type_v = is_result_type<Ty>::value;

    template<typename T>
    struct unwrap_result_type
    {
        using type = T;
    };

    template<typename T>
    struct unwrap_result_type<result<T>>
    {
        using type = T;
    };

    template<typename T>
    using unwrap_result_type_t = typename unwrap_result_type<T>::type;
}