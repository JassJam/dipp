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
        template<service_descriptor_type DescTy> void add_service(size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();

            emplace_or_override(service_type, key, { DescTy{} });
        }

        template<service_descriptor_type DescTy> void add_service(DescTy descriptor, size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();

            emplace_or_override(service_type, key, { std::move(descriptor) });
        }

    public:
        template<service_descriptor_type DescTy> bool emplace_service(size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();

            return emplace(service_type, key, { DescTy{} });
        }

        template<service_descriptor_type DescTy> bool emplace_service(DescTy descriptor, size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();

            return emplace(service_type, key, { std::move(descriptor) });
        }

    public:
        template<service_descriptor_type DescTy, service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        [[nodiscard]] auto get_service(typename DescTy::scope_type& scope, SingletonMemTy& singleton_storage,
                                       ScopedMemTy& scoped_storage, size_t key) -> typename DescTy::service_type
        {
            using value_type   = typename DescTy::value_type;
            using service_type = typename DescTy::service_type;

            auto handle = make_type_key(typeid(service_type).hash_code(), key);
            auto it     = m_Services.find(handle);

            if (it == m_Services.end()) [[unlikely]]
            {
                details::fail<service_not_found, service_type>();
            }

            auto& info = it->second;
            if constexpr (DescTy::lifetime == service_lifetime::singleton)
            {
                auto instance_iter = singleton_storage.find(handle);

                if (instance_iter == nullptr)
                {
                    auto descriptor = std::any_cast<DescTy>(&info.Descriptor);
                    if (!descriptor) [[unlikely]]
                    {
                        details::fail<incompatible_service_descriptor, service_type>();
                    }

                    instance_iter = singleton_storage.emplace(handle, { descriptor->load(scope) });
                }

                return service_type{ std::any_cast<value_type&>(instance_iter->Instance) };
            }
            else if constexpr (DescTy::lifetime == service_lifetime::scoped)
            {
                auto instance_iter = scoped_storage.find(handle);

                if (instance_iter == nullptr)
                {
                    auto descriptor = std::any_cast<DescTy>(&info.Descriptor);
                    if (!descriptor) [[unlikely]]
                    {
                        details::fail<incompatible_service_descriptor, service_type>();
                    }

                    instance_iter = scoped_storage.emplace(handle, { descriptor->load(scope) });
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
        template<service_descriptor_type DescTy> [[nodiscard]] bool has_service(size_t key) const noexcept
        {
            using value_type   = typename DescTy::value_type;
            using service_type = typename DescTy::service_type;

            auto handle = make_type_key(typeid(service_type).hash_code(), key);
            return m_Services.find(handle) != m_Services.end();
        }

    private:
        void emplace_or_override(size_t service_type, size_t hash, typename policy_type::service_info info)
        {
            auto service_handle = make_type_key(service_type, hash);
            auto iter           = m_Services.find(service_handle);

            if (iter != m_Services.end())
            {
                iter->second = std::move(info);
            }
            else
            {
                m_Services.emplace(service_handle, std::move(info));
            }
        }

        bool emplace(size_t service_type, size_t hash, typename policy_type::service_info info)
        {
            auto service_handle = make_type_key(service_type, hash);
            auto iter           = m_Services.find(service_handle);
            return iter == m_Services.end();
        }

    private:
        typename policy_type::service_map_type m_Services;
    };
} // namespace dipp