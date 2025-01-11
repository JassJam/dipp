#pragma once

#include "storage.hpp"
#include "memory.hpp"

namespace dipp
{
    template<service_policy_type StoragePolicyTy, instance_policy_type SingletonPolicyTy,
             instance_policy_type ScopedPolicyTy>
    class service_scope
    {
    public:
        using storage_type           = service_storage<StoragePolicyTy>;
        using singleton_storage_type = service_storage_memory<SingletonPolicyTy>;
        using scoped_storage_type    = service_storage_memory<ScopedPolicyTy>;

    public:
        service_scope(storage_type* storage, singleton_storage_type* singleton_storage) :
            m_Storage(storage), m_SingletonStorage(singleton_storage)
        {
        }

    public:
        template<base_injected_type InjectableTy> [[nodiscard]] auto get() -> InjectableTy
        {
            return m_Storage->get_service<typename InjectableTy::descriptor_type, InjectableTy::key>(
                *this, *m_SingletonStorage, m_LocalStorage);
        }

        template<service_descriptor_type DescTy, string_hash key = string_hash<0>{}>
        [[nodiscard]] auto get() -> typename DescTy::service_type
        {
            return m_Storage->get_service<DescTy, key>(*this, *m_SingletonStorage, m_LocalStorage);
        }

    public:
        template<base_injected_type InjectableTy> [[nodiscard]] bool has() const noexcept
        {
            return m_Storage->has_service<typename InjectableTy::descriptor_type, InjectableTy::key>();
        }

        template<service_descriptor_type DescTy, string_hash key = string_hash<0>{}>
        [[nodiscard]] bool has() const noexcept
        {
            return m_Storage->has_service<DescTy, key>();
        }

    private:
        storage_type*           m_Storage;
        singleton_storage_type* m_SingletonStorage;
        scoped_storage_type     m_LocalStorage;
    };

    using default_service_scope =
        service_scope<default_service_policy, default_instance_policy, default_instance_policy>;

    static_assert(service_scope_type<default_service_scope>, "default_service_scope is not a service_scope_type");
} // namespace dipp