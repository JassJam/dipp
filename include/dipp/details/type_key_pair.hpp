#pragma once

#include <utility>

namespace dipp
{
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