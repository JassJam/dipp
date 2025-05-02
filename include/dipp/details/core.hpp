#pragma once

#include <any>
#include <concepts>
#include <map>
#include <string>
#include <string_view>
#include <typeindex>
#include <functional>
#include <memory>
#include "move_only_any.hpp"

namespace dipp
{
    enum class service_lifetime : uint8_t
    {
        singleton,
        transient,
        scoped
    };

    /// <summary>
    /// Converts a service_lifetime enum to a string_view.
    /// </summary>
    constexpr std::string_view to_string(service_lifetime lifetime)
    {
        switch (lifetime)
        {
            case service_lifetime::singleton:
                return "singleton";
            case service_lifetime::transient:
                return "transient";
            case service_lifetime::scoped:
                return "scoped";
            default:
                return "unknown";
        }
    }

    //

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

    //

    // type is the type of the service/descriptor/instance
    // key is the key identifier of the service/descriptor/instance
    using type_key_pair = std::pair<size_t /*type*/, size_t /*key*/>;

    /// <summary>
    /// Creates a type_key_pair from a type and a key.
    /// </summary>
    [[nodiscard]] constexpr auto make_type_key(size_t type, size_t key) noexcept
    {
        return std::make_pair(type, key);
    }
} // namespace dipp