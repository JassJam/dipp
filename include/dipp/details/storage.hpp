#pragma once

#include "core.hpp"
#include "policy.hpp"
#include "exception.hpp"
#include "move_only_any.hpp"

namespace dipp
{
    template<service_policy_type PolicyTy>
    class service_storage
    {
    public:
        template<service_scope_type ScopeTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        struct service_loader
        {
            ScopeTy& scope;
            SingletonMemTy& singleton_storage;
            ScopedMemTy& scoped_storage;

            template<service_descriptor_type DescTy>
            [[nodiscard]] auto load(move_only_any& service, type_key_pair handle) ->
                typename DescTy::service_type
            {
                return load_service_impl<DescTy>(
                    service, handle, scope, singleton_storage, scoped_storage);
            }
        };

    public:
        using policy_type = PolicyTy;
        using service_map_type = typename policy_type::service_map_type;

    public:
        /// <summary>
        /// Clears all services from the storage.
        /// </summary>
        void clear() noexcept
        {
            m_Services.clear();
        }

        /// <summary>
        /// Clears the service with the specified key from the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        void clear(size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();
            auto handle = make_type_key(service_type, key);
            auto iter = m_Services.find(handle);

            if (iter != m_Services.end())
            {
                m_Services.erase(iter);
            }
        }

        /// <summary>
        /// Clears all services of the specified type from the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        void clear_all()
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();
            for (auto iter = m_Services.begin(); iter != m_Services.end();)
            {
                if (iter->first.first == service_type)
                {
                    iter = m_Services.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

    public:
        /// <summary>
        /// Adds a service to the storage with the specified key.
        /// </summary>
        template<service_descriptor_type DescTy>
        void add_service(DescTy&& descriptor, size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();
            auto service_handle = make_type_key(service_type, key);

            m_Services[service_handle].emplace_back(std::forward<DescTy>(descriptor));
        }

    public:
        /// <summary>
        /// Adds a service to the storage with the specified key if it does not already exist.
        /// </summary>
        template<service_descriptor_type DescTy>
        bool emplace_service(DescTy&& descriptor, size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();

            auto service_handle = make_type_key(service_type, key);
            auto iter = m_Services.find(service_handle);
            if (iter != m_Services.end())
            {
                return false;
            }

            m_Services[service_handle].emplace_back(std::forward<DescTy>(descriptor));
            return true;
        }

    public:
        /// <summary>
        /// Gets a service from the storage with the specified key.
        /// </summary>
        template<service_descriptor_type DescTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        [[nodiscard]] auto get_service(typename DescTy::scope_type& scope,
                                       SingletonMemTy& singleton_storage,
                                       ScopedMemTy& scoped_storage,
                                       size_t key) -> typename DescTy::service_type
        {
            using service_type = typename DescTy::service_type;

            auto service_handle = typeid(typename DescTy::service_type).hash_code();
            auto handle = make_type_key(service_handle, key);
            auto it = m_Services.find(handle);

            if (it == m_Services.end()) [[unlikely]]
            {
                details::fail<service_not_found, service_type>();
            }

            auto& last_service = it->second.back();

            service_loader loader{scope, singleton_storage, scoped_storage};
            return loader.load<DescTy>(last_service, handle);
        }

    public:
        /// <summary>
        /// Checks if a service with the specified key exists in the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] bool has_service(size_t key) const noexcept
        {
            using value_type = typename DescTy::value_type;
            using service_type = typename DescTy::service_type;

            auto service_handle = typeid(typename DescTy::service_type).hash_code();
            auto handle = make_type_key(service_handle, key);

            return m_Services.find(handle) != m_Services.end();
        }

    public:
        /// <summary>
        /// Counts the number of services with the specified key in the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] size_t count(size_t key) const noexcept
        {
            using service_type = typename DescTy::service_type;

            auto service_handle = typeid(service_type).hash_code();
            auto handle = make_type_key(service_handle, key);
            auto it = m_Services.find(handle);

            return it != m_Services.end() ? it->second.size() : 0;
        }

        /// <summary>
        /// Counts the total number of services of the specified type in the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] size_t count_all() const noexcept
        {
            using service_type = typename DescTy::service_type;

            auto service_handle = typeid(service_type).hash_code();
            size_t count = 0;

            for (auto iter = m_Services.begin(); iter != m_Services.end(); ++iter)
            {
                if (iter->first.first == service_handle)
                {
                    count += iter->second.size();
                }
            }

            return count;
        }

        /// <summary>
        /// Iterates over all services of the specified type in the storage and applies the provided
        /// function to each service.
        /// </summary>
        template<service_descriptor_type DescTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy,
                 typename FuncTy>
        void for_each(FuncTy&& func,
                      typename DescTy::scope_type& scope,
                      SingletonMemTy& singleton_storage,
                      ScopedMemTy& scoped_storage,
                      size_t key)
        {
            using service_type = typename DescTy::service_type;

            auto service_handle = typeid(typename DescTy::service_type).hash_code();
            auto handle = make_type_key(service_handle, key);
            auto it = m_Services.find(handle);

            if (it != m_Services.end())
            {
                service_loader loader{scope, singleton_storage, scoped_storage};
                for (auto& service : it->second)
                {
                    func(loader.load<DescTy>(service, handle));
                }
            }
        }

        /// <summary>
        /// Iterates over all services of the specified type in the storage and applies the provided
        /// </summary>
        template<service_descriptor_type DescTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy,
                 typename FuncTy>
        void for_each_all(FuncTy&& func,
                          typename DescTy::scope_type& scope,
                          SingletonMemTy& singleton_storage,
                          ScopedMemTy& scoped_storage)
        {
            using service_type = typename DescTy::service_type;

            auto service_handle = typeid(service_type).hash_code();
            service_loader loader{scope, singleton_storage, scoped_storage};

            for (auto iter = m_Services.begin(); iter != m_Services.end(); ++iter)
            {
                if (iter->first.first == service_handle)
                {
                    for (auto& service : iter->second)
                    {
                        func(loader.load<DescTy>(service, iter->first));
                    }
                }
            }
        }

    private:
        /// <summary>
        /// Loads a service from the storage with the specified key.
        /// </summary>
        template<service_descriptor_type DescTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        [[nodiscard]] static auto load_service_impl(move_only_any& service,
                                                    type_key_pair handle,
                                                    typename DescTy::scope_type& scope,
                                                    SingletonMemTy& singleton_storage,
                                                    ScopedMemTy& scoped_storage) ->
            typename DescTy::service_type
        {
            using service_type = typename DescTy::service_type;
            using value_type = typename DescTy::value_type;

            if constexpr (DescTy::lifetime == service_lifetime::singleton)
            {
                auto instance_iter = singleton_storage.find(handle);

                if (instance_iter == nullptr)
                {
                    auto descriptor = service.cast<DescTy>();
                    if (!descriptor) [[unlikely]]
                    {
                        details::fail<incompatible_service_descriptor, service_type>();
                    }

                    instance_iter = singleton_storage.emplace(handle, *descriptor, scope);
                }

                return service_type{*instance_iter->cast<value_type>()};
            }
            else if constexpr (DescTy::lifetime == service_lifetime::scoped)
            {
                auto instance_iter = scoped_storage.find(handle);

                if (instance_iter == nullptr)
                {
                    auto descriptor = service.cast<DescTy>();
                    if (!descriptor) [[unlikely]]
                    {
                        details::fail<incompatible_service_descriptor, service_type>();
                    }

                    instance_iter = scoped_storage.emplace(handle, *descriptor, scope);
                }

                return service_type{*instance_iter->cast<value_type>()};
            }
            else if constexpr (DescTy::lifetime == service_lifetime::transient)
            {
                auto descriptor = service.cast<DescTy>();
                if (!descriptor) [[unlikely]]
                {
                    details::fail<incompatible_service_descriptor, service_type>();
                }

                return service_type{std::move(*descriptor->load(scope).cast<value_type>())};
            }
            else
            {
                details::unreachable();
            }
        }

    private:
        service_map_type m_Services;
    };
} // namespace dipp