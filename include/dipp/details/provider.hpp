#pragma once

#include "scope.hpp"
#include "result.hpp"

namespace dipp::details
{
    template<service_policy_type StoragePolicyTy,
             service_storage_memory_type SingletonPolicyTy,
             service_storage_memory_type ScopedPolicyTy>
    class base_service_provider
    {
    public:
        using singleton_memory_type = SingletonPolicyTy;
        using storage_type = base_service_storage<StoragePolicyTy>;
        using scope_type = base_service_scope<StoragePolicyTy, SingletonPolicyTy, ScopedPolicyTy>;
        using collection_type = base_service_collection<StoragePolicyTy>;

    public:
        explicit base_service_provider(collection_type collection) noexcept(
            std::is_nothrow_move_constructible_v<storage_type>)
            : m_Storage(std::move(collection.m_Storage))
            , m_RootScope(&m_Storage, &m_SingletonStorage)
        {
        }

        base_service_provider(base_service_provider&& services) noexcept
            : m_SingletonStorage(std::move(services.m_SingletonStorage))
            , m_Storage(std::move(services.m_Storage))
            , m_RootScope(&m_Storage, &m_SingletonStorage, std::move(services.m_RootScope))
        {
        }

        base_service_provider& operator=(base_service_provider&& services)
        {
            if (this != &services)
            {
                m_SingletonStorage = std::move(services.m_SingletonStorage);
                m_Storage = std::move(services.m_Storage);
                m_RootScope =
                    scope_type(&m_Storage, &m_SingletonStorage, std::move(services.m_RootScope));
            }
            return *this;
        }

        base_service_provider(const base_service_provider&) = delete;
        base_service_provider& operator=(const base_service_provider&) = delete;

        ~base_service_provider() = default;

    public:
        /// <summary>
        /// Returns the collection of services as a copy.
        /// </summary>
        [[nodiscard]] auto collection() const noexcept(noexcept(collection_type{m_Storage}))
        {
            return collection_type{m_Storage};
        }

        /// <summary>
        /// Creates a new scope for the service provider.
        /// </summary>
        [[nodiscard]] auto create_scope()
        {
            return scope_type(&m_Storage, &m_SingletonStorage);
        }

        /// <summary>
        /// Returns the root scope of the service provider.
        /// </summary>
        [[nodiscard]] auto& root_scope() noexcept
        {
            return m_RootScope;
        }
        /// <summary>
        /// Returns the root scope of the service provider.
        /// </summary>
        [[nodiscard]] auto& root_scope() const noexcept
        {
            return m_RootScope;
        }

    public:
        /// <summary>
        /// Gets the service of the specified type from the root scope.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] auto get() -> result<InjectableTy>
        {
            return root_scope().template get<InjectableTy>();
        }

    public:
        /// <summary>
        /// Checks if the service of the specified type is registered in the root scope.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has() const noexcept
        {
            return root_scope().template has<InjectableTy>();
        }

    public:
        /// <summary>
        /// Counts the number of services of the specified type in the root scope.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count() const noexcept
        {
            return root_scope().template count<InjectableTy>();
        }

    public:
        /// <summary>
        /// Counts the number of services of the specified type in all scopes.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count_all() const noexcept
        {
            return root_scope().template count_all<InjectableTy>();
        }

    public:
        /// <summary>
        /// Gets all services from the storage with the specified type and key.
        /// </summary>
        template<base_injected_type InjectableTy,
                 container_type ContainerTy = std::vector<dipp::details::result<InjectableTy>>>
        ContainerTy get_all()
        {
            return root_scope().template get_all<InjectableTy, ContainerTy>();
        }

    private:
        singleton_memory_type m_SingletonStorage;
        storage_type m_Storage;
        scope_type m_RootScope;
    };

    using service_provider = base_service_provider<default_service_policy,
                                                   default_service_storage_memory_type,
                                                   default_service_storage_memory_type>;

    static_assert(service_provider_type<service_provider>,
                  "default_service_provider is not a service_provider");
}