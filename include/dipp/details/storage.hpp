#pragma once

#include "loader.hpp"
#include "getter.hpp"

namespace dipp::details
{
    template<service_policy_type PolicyTy>
    class base_service_storage
    {
    public:
        using policy_type = PolicyTy;
        using service_map_type = typename policy_type::service_map_type;

    public:
        /// <summary>
        /// Clears all services from the storage.
        /// </summary>
        void clear() noexcept
        {
            m_Descriptors.clear();
        }

        /// <summary>
        /// Clears the service with the specified key from the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        void clear(size_t key)
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();
            auto handle = make_type_key(service_type, key);
            auto iter = m_Descriptors.find(handle);

            if (iter != m_Descriptors.end())
            {
                m_Descriptors.erase(iter);
            }
        }

        /// <summary>
        /// Clears all services of the specified type from the storage.
        /// </summary>
        template<service_descriptor_type DescTy>
        void clear_all()
        {
            auto service_type = typeid(typename DescTy::service_type).hash_code();
            for (auto iter = m_Descriptors.begin(); iter != m_Descriptors.end();)
            {
                if (iter->first.first == service_type)
                {
                    iter = m_Descriptors.erase(iter);
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

            m_Descriptors[service_handle].emplace_back(
                move_only_any::make<DescTy>(std::forward<DescTy>(descriptor)));
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
            auto iter = m_Descriptors.find(service_handle);
            if (iter != m_Descriptors.end())
            {
                return false;
            }

            m_Descriptors[service_handle].emplace_back(
                move_only_any::make<DescTy>(std::forward<DescTy>(descriptor)));
            return true;
        }

    public:
        /// <summary>
        /// Gets a service from the storage with the specified key.
        /// </summary>
        template<base_injected_type InjectableTy,
                 service_scope_type ScopeTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        [[nodiscard]] auto get_service(ScopeTy& scope,
                                       SingletonMemTy& singleton_storage,
                                       ScopedMemTy& scoped_storage) -> result<InjectableTy>
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;

            auto service_handle = typeid(service_type).hash_code();
            auto handle = make_type_key(service_handle, InjectableTy::key);
            auto it = m_Descriptors.find(handle);

            if (it == m_Descriptors.end()) [[unlikely]]
            {
                DIPP_RETURN_ERROR(service_not_found::error<service_type>());
            }

            auto& last_service = it->second.back();

            service_loader loader{scope, singleton_storage, scoped_storage};
            return loader.load<InjectableTy>(last_service);
        }

    public:
        /// <summary>
        /// Checks if a service with the specified key exists in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has_service() const noexcept
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using value_type = typename descriptor_type::value_type;
            using service_type = typename descriptor_type::service_type;

            auto service_handle = typeid(service_type).hash_code();
            auto handle = make_type_key(service_handle, InjectableTy::key);

            return m_Descriptors.find(handle) != m_Descriptors.end();
        }

    public:
        /// <summary>
        /// Counts the number of services with the specified key in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count() const noexcept
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;

            auto service_handle = typeid(service_type).hash_code();
            auto handle = make_type_key(service_handle, InjectableTy::key);
            auto it = m_Descriptors.find(handle);

            return it != m_Descriptors.end() ? it->second.size() : 0;
        }

        /// <summary>
        /// Counts the total number of services of the specified type in the storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] size_t count_all() const noexcept
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;

            auto service_handle = typeid(service_type).hash_code();
            size_t count = 0;

            for (auto iter = m_Descriptors.begin(); iter != m_Descriptors.end(); ++iter)
            {
                if (iter->first.first == service_handle)
                {
                    count += iter->second.size();
                }
            }

            return count;
        }

    public:
        /// <summary>
        /// Finds all services from the storage with the specified type and key.
        /// </summary>
        template<base_injected_type InjectableTy,
                 typename FnTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
            requires std::
                is_invocable_v<FnTy, base_service_getter<InjectableTy, SingletonMemTy, ScopedMemTy>>
            void find_all(typename InjectableTy::descriptor_type::scope_type& scope,
                          FnTy&& callback,
                          SingletonMemTy& singleton_storage,
                          ScopedMemTy& scoped_storage)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;
            using service_getter_type =
                base_service_getter<InjectableTy, SingletonMemTy, ScopedMemTy>;

            auto service_handle = typeid(service_type).hash_code();
            auto handle = make_type_key(service_handle, InjectableTy::key);
            auto it = m_Descriptors.find(handle);

            if (it == m_Descriptors.end())
            {
                return;
            }

            service_loader loader{scope, singleton_storage, scoped_storage};
            for (auto& service : it->second)
            {
                service_getter_type getter{loader, service};
                callback(getter);
            }
        }

    private:
        service_map_type m_Descriptors;
    };
}