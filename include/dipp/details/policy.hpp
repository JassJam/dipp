#pragma once

#include <concepts>
#include <any>
#include <map>
#include "core.hpp"
#include "move_only_any.hpp"

namespace dipp
{
    struct default_service_policy
    {
        using service_info = std::vector<move_only_any>;
        using service_map_type = std::map<type_key_pair, service_info>;
    };
    static_assert(service_policy_type<default_service_policy>,
                  "default_service_policy is not a service_policy_type");

    struct default_service_storage_memory_type
    {
    public:
        struct instance_info
        {
            template<service_descriptor_type DescTy, service_scope_type ScopeTy>
            instance_info(DescTy& descriptor, ScopeTy& scope)
                : Instance(descriptor.load(scope))
            {
            }

            template<typename Ty>
            constexpr Ty* cast() noexcept
            {
                return Instance.cast<Ty>();
            }

            move_only_any Instance;
        };

        using instance_map_type = std::map<type_key_pair, instance_info>;

    public:
        default_service_storage_memory_type() = default;

        default_service_storage_memory_type(const default_service_storage_memory_type&) = delete;
        default_service_storage_memory_type& operator=(const default_service_storage_memory_type&) =
            delete;

        default_service_storage_memory_type(default_service_storage_memory_type&&) = default;
        default_service_storage_memory_type& operator=(default_service_storage_memory_type&&) =
            default;

        ~default_service_storage_memory_type()
        {
            for (size_t i = 0; i < m_Instances.size(); i++)
            {
                m_Instances[m_Instances.size() - i - 1].reset();
            }
        }

    public:
        auto find(const type_key_pair& handle)
        {
            auto it = m_InstanceRefs.find(handle);
            return it != m_InstanceRefs.end() ? it->second : nullptr;
        }

        template<service_descriptor_type DescTy, service_scope_type ScopeTy>
        auto emplace(const type_key_pair& handle, DescTy& descriptor, ScopeTy& scope)
        {
            auto instance = std::make_unique<instance_info>(descriptor, scope);
            auto instance_ptr = instance.get();

            m_Instances.emplace_back(std::move(instance));
            auto it = m_InstanceRefs.emplace(handle, instance_ptr).first;

            return it->second;
        }

    private:
        std::vector<std::unique_ptr<instance_info>> m_Instances;
        std::map<type_key_pair, instance_info*> m_InstanceRefs;
    };
    static_assert(service_storage_memory_type<default_service_storage_memory_type>,
                  "default_service_storage_memory_type is not a service_storage_memory_type");
} // namespace dipp