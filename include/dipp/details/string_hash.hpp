#pragma once

#include <string_view>

namespace dipp::details
{
    struct string_hash
    {
        constexpr string_hash() noexcept
            : value(0)
        {
        }

        constexpr string_hash(std::string_view str)
            : value(compute_hash(str))
        {
        }

        constexpr operator size_t() const noexcept
        {
            return value;
        }

        size_t value;

    private:
        static constexpr size_t compute_hash(std::string_view str) noexcept
        {
            size_t hash = 0;
            for (char c : str)
            {
                hash = hash * 31 + static_cast<size_t>(c);
            }
            return hash;
        }
    };

    /// <summary>
    /// Generates a hash key from a string literal or std::string_view.
    /// </summary>
    static constexpr size_t key(const char* str) noexcept
    {
        return string_hash(str).value;
    }

    /// <summary>
    /// Generates a hash key from a std::string_view.
    /// </summary>
    static constexpr size_t key(const std::string_view& str) noexcept
    {
        return string_hash(str).value;
    }
}