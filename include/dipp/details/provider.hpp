#pragma once

#include "scope.hpp"

namespace dipp
{
    template<service_policy_type StoragePolicyTy,
             service_storage_memory_type SingletonPolicyTy,
             service_storage_memory_type ScopedPolicyTy>
    class service_provider
    {
    public:
        using singleton_memory_type = SingletonPolicyTy;
        using storage_type = service_storage<StoragePolicyTy>;
        using scope_type = service_scope<StoragePolicyTy, SingletonPolicyTy, ScopedPolicyTy>;
        using collection_type = service_collection<StoragePolicyTy>;

    public:
        explicit service_provider(collection_type collection) noexcept(
            std::is_nothrow_move_constructible_v<storage_type>)
            : m_Storage(std::move(collection.m_Storage))
            , m_RootScope(&m_Storage, &m_SingletonStorage)
        {
        }

        service_provider(service_provider&& services) noexcept
            : m_Storage(std::move(services.m_Storage))
            , m_RootScope(&m_Storage, &m_SingletonStorage, std::move(services.m_RootScope))
        {
        }

        service_provider& operator=(service_provider&& services)
        {
            if (this != &services)
            {
                m_Storage = std::move(services.m_Storage);
                m_RootScope =
                    scope_type(&m_Storage, &m_SingletonStorage, std::move(services.m_RootScope));
            }
            return *this;
        }

        service_provider(const service_provider&) = delete;
        service_provider& operator=(const service_provider&) = delete;

        ~service_provider() = default;

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

        /// <summary>
        /// Gets the service of the specified type from the root scope.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] auto get(size_t key = {}) -> typename DescTy::service_type
        {
            return root_scope().template get<DescTy>(key);
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

        /// <summary>
        /// Checks if the service of the specified type is registered in the root scope.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] bool has(size_t key = {}) const noexcept
        {
            return root_scope().template has<DescTy>(key);
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

        /// <summary>
        /// Counts the number of services of the specified type in the root scope.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] size_t count(size_t key = {}) const noexcept
        {
            using descriptor_type = DescTy;
            return root_scope().template count<descriptor_type>(key);
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

        /// <summary>
        /// Counts the number of services of the specified type in all scopes.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] size_t count_all() const noexcept
        {
            using descriptor_type = DescTy;
            return root_scope().template count_all<descriptor_type>();
        }

    public:
        /// <summary>
        /// Creates a new service of the specified type in the root scope.
        /// </summary>
        template<base_injected_type InjectableTy, typename FuncTy>
        [[nodiscard]] void for_each(FuncTy&& func)
        {
            root_scope().template for_each<InjectableTy>(std::forward<FuncTy>(func));
        }

        /// <summary>
        /// Creates a new service of the specified type in the root scope.
        /// </summary>
        template<service_descriptor_type DescTy, typename FuncTy>
        void for_each(FuncTy&& func, size_t key = {})
        {
            using descriptor_type = DescTy;
            root_scope().template for_each<descriptor_type>(std::forward<FuncTy>(func), key);
        }

    public:
        /// <summary>
        /// Creates a new service of the specified type in all scopes.
        /// </summary>
        template<base_injected_type InjectableTy, typename FuncTy>
        [[nodiscard]] void for_each_all(FuncTy&& func)
        {
            root_scope().template for_each_all<InjectableTy>(std::forward<FuncTy>(func));
        }

        /// <summary>
        /// Creates a new service of the specified type in all scopes.
        /// </summary>
        template<service_descriptor_type DescTy, typename FuncTy>
        void for_each_all(FuncTy&& func)
        {
            using descriptor_type = DescTy;
            root_scope().template for_each_all<descriptor_type>(std::forward<FuncTy>(func));
        }

    private:
        singleton_memory_type m_SingletonStorage;
        storage_type m_Storage;
        scope_type m_RootScope;
    };

    using default_service_provider = service_provider<default_service_policy,
                                                      default_service_storage_memory_type,
                                                      default_service_storage_memory_type>;

    static_assert(service_provider_type<default_service_provider>,
                  "default_service_provider is not a service_provider");
} // namespace dipp