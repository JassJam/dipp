#pragma once

#include <any>
#include <concepts>
#include <map>
#include <string>
#include <typeindex>
#include <functional>
#include <memory>

namespace dipp
{
    enum class service_lifetime : uint8_t
    {
        singleton,
        transient,
        scoped
    };

    template<size_t N> struct string_literal
    {
        char data[N];

        constexpr string_literal(const char (&str)[N]) noexcept
        {
            for (size_t i = 0; i < N; ++i)
            {
                data[i] = str[i];
            }
        }
        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return N;
        }
        [[nodiscard]] constexpr const char* c_str() const noexcept
        {
            return data;
        }
    };

    template<> struct string_literal<0>
    {
        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return 0;
        }
        [[nodiscard]] constexpr const char* c_str() const noexcept
        {
            return "";
        }
    };
} // namespace dipp
