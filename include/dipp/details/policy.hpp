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

        using service_map_type = std::map<type_key_pair, service_info>;
    };
    static_assert(service_policy_type<default_service_policy>, "default_service_policy is not a service_policy_type");

    struct default_instance_policy
    {
        struct instance_info
        {
            std::any Instance;
        };

        using instance_map_type = std::map<type_key_pair, instance_info>;
    };
    static_assert(instance_policy_type<default_instance_policy>,
                  "default_instance_policy is not a instance_policy_type");
} // namespace dipp