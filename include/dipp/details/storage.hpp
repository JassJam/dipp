#pragma once

#include "core.hpp"
#include "memory.hpp"
#include "exception.hpp"

namespace dipp
{
    template<service_policy_type PolicyTy> class service_storage
    {
    public:
        using policy_type = PolicyTy;

    public:
        template<service_descriptor_type DescTy, string_hash key = string_hash<0>{}> void add_service()
        {
            using service_type = typename DescTy::service_type;
            emplace_or_override(typeid(service_type).hash_code(), key.hash(), { DescTy{} });
        }

        template<service_descriptor_type DescTy, string_hash key = string_hash<0>{}> void add_service(DescTy descriptor)
        {
            using service_type = typename DescTy::service_type;
            emplace_or_override(typeid(service_type).hash_code(), key.hash(), { std::move(descriptor) });
        }

    public:
        template<service_descriptor_type DescTy, string_hash key = string_hash<0>{},
                 service_storage_memory_type SingletonMemTy, service_storage_memory_type ScopedMemTy>
        [[nodiscard]] auto get_service(typename DescTy::scope_type& scope, SingletonMemTy& singleton_storage,
                                       ScopedMemTy& scoped_storage) -> typename DescTy::service_type
        {
            using value_type   = typename DescTy::value_type;
            using service_type = typename DescTy::service_type;

            auto handle = policy_type::make_key(typeid(service_type).hash_code(), key.hash());
            auto it     = m_Services.find(handle);

            if (it == m_Services.end()) [[unlikely]]
            {
                details::fail<service_not_found, service_type>();
            }

            auto& info = it->second;
            if constexpr (DescTy::lifetime == service_lifetime::singleton)
            {
                auto singleton_handle =
                    SingletonMemTy::policy_type::make_key(typeid(service_type).hash_code(), key.hash());
                auto instance_iter = singleton_storage.find(singleton_handle);

                if (instance_iter == nullptr)
                {
                    auto descriptor = std::any_cast<DescTy>(&info.Descriptor);
                    if (!descriptor) [[unlikely]]
                    {
                        details::fail<incompatible_service_descriptor, service_type>();
                    }

                    instance_iter = singleton_storage.emplace(singleton_handle, { descriptor->load(scope) });
                }

                return service_type{ std::any_cast<value_type&>(instance_iter->Instance) };
            }
            else if constexpr (DescTy::lifetime == service_lifetime::scoped)
            {
                auto scoped_handle = ScopedMemTy::policy_type::make_key(typeid(service_type).hash_code(), key.hash());
                auto instance_iter = scoped_storage.find(scoped_handle);

                if (instance_iter == nullptr)
                {
                    auto descriptor = std::any_cast<DescTy>(&info.Descriptor);
                    if (!descriptor) [[unlikely]]
                    {
                        details::fail<incompatible_service_descriptor, service_type>();
                    }

                    instance_iter = scoped_storage.emplace(scoped_handle, { descriptor->load(scope) });
                }

                return service_type{ std::any_cast<value_type&>(instance_iter->Instance) };
            }
            else if constexpr (DescTy::lifetime == service_lifetime::transient)
            {
                auto descriptor = std::any_cast<DescTy>(&info.Descriptor);
                if (!descriptor) [[unlikely]]
                {
                    details::fail<incompatible_service_descriptor, service_type>();
                }

                return service_type{ descriptor->load(scope) };
            }
            else
            {
                details::unreachable();
            }
        }

    public:
        template<service_descriptor_type DescTy, string_hash key = string_hash<0>{}>
        [[nodiscard]] bool has_service() const noexcept
        {
            using value_type   = typename DescTy::value_type;
            using service_type = typename DescTy::service_type;

            auto handle = policy_type::make_key(typeid(service_type).hash_code(), key.hash());
            return m_Services.contains(handle);
        }

    private:
        void emplace_or_override(size_t type, size_t hash, policy_type::service_info info)
        {
            auto handle = policy_type::make_key(type, hash);
            auto iter   = m_Services.find(handle);

            if (iter != m_Services.end())
            {
                iter->second = std::move(info);
            }
            else
            {
                m_Services.emplace(handle, std::move(info));
            }
        }

    private:
        typename policy_type::service_map_type m_Services;
    };
} // namespace dipp