#pragma once

#include "scope.hpp"

namespace dipp
{
    template<service_policy_type StoragePolicyTy, instance_policy_type SingletonPolicyTy,
             instance_policy_type ScopedPolicyTy>
    class service_provider
    {
    public:
        using singleton_memory_type = service_storage_memory<SingletonPolicyTy>;
        using storage_type          = service_storage<StoragePolicyTy>;
        using scope_type            = service_scope<StoragePolicyTy, SingletonPolicyTy, ScopedPolicyTy>;
        using collection_type       = service_collection<StoragePolicyTy>;

    public:
        service_provider(collection_type collection) :
            m_Storage(std::move(collection.m_Storage)), m_RootScope(&m_Storage, &m_SingletonStorage)
        {
        }

    public:
        [[nodiscard]] auto create_scope()
        {
            return scope_type(&m_Storage, &m_SingletonStorage);
        }

        [[nodiscard]] auto& root_scope() noexcept
        {
            return m_RootScope;
        }

        [[nodiscard]] auto& root_scope() const noexcept
        {
            return m_RootScope;
        }

    public:
        template<base_injected_type InjectableTy> [[nodiscard]] auto get() -> InjectableTy
        {
            return root_scope().template get<InjectableTy>();
        }

        template<service_descriptor_type DescTy>
        [[nodiscard]] auto get(size_t key = {}) -> typename DescTy::service_type
        {
            return root_scope().template get<DescTy>(key);
        }

    public:
        template<base_injected_type InjectableTy> [[nodiscard]] bool has() const noexcept
        {
            return root_scope().template has<InjectableTy>();
        }

        template<service_descriptor_type DescTy> [[nodiscard]] bool has(size_t key = {}) const noexcept
        {
            return root_scope().template has<DescTy>(key);
        }

    private:
        singleton_memory_type m_SingletonStorage;
        storage_type          m_Storage;
        scope_type            m_RootScope;
    };

    using default_service_provider =
        service_provider<default_service_policy, default_instance_policy, default_instance_policy>;

    static_assert(service_provider_type<default_service_provider>,
                  "default_service_provider is not a service_provider");
} // namespace dipp