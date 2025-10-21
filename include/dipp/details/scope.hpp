#pragma once

#include "storage.hpp"
#include "policy.hpp"
#include "string_hash.hpp"

namespace dipp::details
{
    template<service_policy_type StoragePolicyTy,
             service_storage_memory_type SingletonPolicyTy,
             service_storage_memory_type ScopedPolicyTy>
    class base_service_scope
    {
    public:
        using storage_type = base_service_storage<StoragePolicyTy>;
        using singleton_storage_type = SingletonPolicyTy;
        using scoped_storage_type = ScopedPolicyTy;

    public:
        base_service_scope(storage_type* storage, singleton_storage_type* singleton_storage)
            : m_Storage(storage)
            , m_SingletonStorage(singleton_storage)
        {
        }

        base_service_scope(storage_type* storage,
                           singleton_storage_type* singleton_storage,
                           base_service_scope&& scope)
            : m_Storage(storage)
            , m_SingletonStorage(singleton_storage)
            , m_LocalStorage(std::move(scope.m_LocalStorage))
        {
        }

        base_service_scope(const base_service_scope&) = delete;
        base_service_scope& operator=(const base_service_scope&) = delete;

        base_service_scope(base_service_scope&&) = default;
        base_service_scope& operator=(base_service_scope&&) = default;

        ~base_service_scope() = default;

    public:
        /// <summary>
        /// Get a service from the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] auto get() -> result<InjectableTy>
        {
            return m_Storage->template get_service<InjectableTy>(
                *this, *m_SingletonStorage, m_LocalStorage);
        }

    public:
        /// <summary>
        /// Check if a service is registered in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has() const noexcept
        {
            return m_Storage->template has_service<InjectableTy>();
        }

    public:
        /// <summary>
        /// Count the number of services registered in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count() const noexcept
        {
            return m_Storage->template count<InjectableTy>();
        }

    public:
        /// <summary>
        /// Count the number of services registered in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count_all() const noexcept
        {
            return m_Storage->template count_all<InjectableTy>();
        }

    public:
        /// <summary>
        /// Finds all services from the storage with the specified type and key.
        /// </summary>
        template<base_injected_type InjectableTy, typename FnTy>
            requires std::is_invocable_v<
                FnTy,
                base_service_getter<InjectableTy, singleton_storage_type, scoped_storage_type>>
        void find_all(FnTy&& callback)
        {
            return m_Storage->template find_all<InjectableTy>(
                *this, std::forward<FnTy>(callback), *m_SingletonStorage, m_LocalStorage);
        }

    private:
        storage_type* m_Storage;
        singleton_storage_type* m_SingletonStorage;
        scoped_storage_type m_LocalStorage;
    };

    using service_scope = base_service_scope<default_service_policy,
                                             default_service_storage_memory_type,
                                             default_service_storage_memory_type>;

    static_assert(service_scope_type<service_scope>,
                  "default_service_scope is not a service_scope_type");
}