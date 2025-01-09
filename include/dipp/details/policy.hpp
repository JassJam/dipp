#pragma once

#include <concepts>
#include <any>
#include <map>
#include "core.hpp"

namespace dipp
{
    struct default_service_policy
    {
        struct service_info
        {
            std::any Descriptor;
        };

        using service_key_type = std::pair<std::type_index, std::string>;
        using service_map_type = std::map<service_key_type, service_info>;

        static auto make_key(std::type_index type, const char* key) noexcept
        {
            return std::make_pair(type, key);
        }
    };
    static_assert(service_policy_type<default_service_policy>, "default_service_policy is not a service_policy_type");

    struct default_instance_policy
    {
        struct instance_info
        {
            std::any Instance;
        };

        using instance_key_type = std::pair<std::type_index, std::string>;
        using instance_map_type = std::map<instance_key_type, instance_info>;

        static auto make_key(std::type_index type, const char* key) noexcept
        {
            return std::make_pair(type, key);
        }
    };
    static_assert(instance_policy_type<default_instance_policy>,
                  "default_instance_policy is not a instance_policy_type");
} // namespace dipp