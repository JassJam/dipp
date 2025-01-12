#pragma once

#include "core.hpp"
#include "policy.hpp"
#include "concepts.hpp"

namespace dipp
{
    template<instance_policy_type PolicyTy> class service_storage_memory
    {
    public:
        using policy_type = PolicyTy;

    public:
        auto find(const typename policy_type::instance_key_type& handle)
        {
            auto it = m_Instances.find(handle);
            return it != m_Instances.end() ? &it->second : nullptr;
        }

        auto emplace(const typename policy_type::instance_key_type& handle,
                     typename policy_type::instance_info            instance)
        {
            auto [iter, success] = m_Instances.emplace(handle, std::move(instance));
            return success ? &iter->second : nullptr;
        }

    private:
        typename policy_type::instance_map_type m_Instances;
    };
} // namespace dipp