#pragma once

#include "policy.hpp"
#include "move_only_any.hpp"

#include "fail.hpp"
#include "errors/service_not_found.hpp"
#include "errors/incompatible_service_descriptor.hpp"
#include "errors/mismatched_service_type.hpp"

namespace dipp::details
{
    template<service_policy_type PolicyTy>
    class base_service_storage
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

            template<base_injected_type InjectableTy>
            [[nodiscard]] auto load(move_only_any& service) -> result<InjectableTy>
            {
                return load_service_impl<InjectableTy>(
                    service, scope, singleton_storage, scoped_storage);
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
        /// Gets all services from the storage with the specified type and key.
        /// </summary>
        template<base_injected_type InjectableTy,
                 container_type ContainerTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        ContainerTy get_all(typename InjectableTy::descriptor_type::scope_type& scope,
                            SingletonMemTy& singleton_storage,
                            ScopedMemTy& scoped_storage)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;

            auto service_handle = typeid(service_type).hash_code();
            auto handle = make_type_key(service_handle, InjectableTy::key);
            auto it = m_Descriptors.find(handle);

            ContainerTy results;
            if (it == m_Descriptors.end())
            {
                return results;
            }

            results.reserve(it->second.size());

            service_loader loader{scope, singleton_storage, scoped_storage};
            for (auto& service : it->second)
            {
                results.emplace_back(loader.load<InjectableTy>(service));
            }

            return results;
        }

    private:
        /// <summary>
        /// Loads a service from the storage with the specified key.
        /// </summary>
        template<base_injected_type InjectableTy,
                 service_storage_memory_type SingletonMemTy,
                 service_storage_memory_type ScopedMemTy>
        [[nodiscard]] static auto load_service_impl(
            move_only_any& service,
            typename InjectableTy::descriptor_type::scope_type& scope,
            SingletonMemTy& singleton_storage,
            ScopedMemTy& scoped_storage) -> result<InjectableTy>
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;

            if constexpr (descriptor_type::lifetime == service_lifetime::singleton)
            {
                return load_mem_service<InjectableTy>(service, scope, singleton_storage);
            }
            else if constexpr (descriptor_type::lifetime == service_lifetime::scoped)
            {
                return load_mem_service<InjectableTy>(service, scope, scoped_storage);
            }
            else if constexpr (descriptor_type::lifetime == service_lifetime::transient)
            {
                return load_transient_service<InjectableTy>(service, scope);
            }
            else
            {
                details::unreachable();
            }
        }

    private:
        /// <summary>
        /// Loads a scoped/singleton service from storage.
        /// </summary>
        template<base_injected_type InjectableTy, service_storage_memory_type MemTy>
        [[nodiscard]] static auto load_mem_service(
            move_only_any& service,
            typename InjectableTy::descriptor_type::scope_type& scope,
            MemTy& storage) -> result<InjectableTy>
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;
            using value_type = typename descriptor_type::value_type;

            auto handle = make_type_key(typeid(descriptor_type).hash_code(),
                                        std::bit_cast<size_t>(std::addressof(service)));
            auto instance_iter = storage.find(handle);

            if (instance_iter == nullptr)
            {
                auto descriptor = service.cast<descriptor_type>();
                if (!descriptor) [[unlikely]]
                {
                    DIPP_RETURN_ERROR(incompatible_service_descriptor::error<service_type>());
                }

                instance_iter = storage.emplace(handle, descriptor->value(), scope);
            }

#ifdef DIPP_USE_RESULT
            if (instance_iter->has_error()) [[unlikely]]
            {
                return instance_iter->error();
            }
#endif

            auto instance = instance_iter->cast<value_type>();
            if (!instance || instance->has_error()) [[unlikely]]
            {
#ifdef DIPP_USE_RESULT
                if (instance)
                {
                    return instance->error();
                }
#endif
                DIPP_RETURN_ERROR(mismatched_service_type::error<service_type>());
            }

            return make_result<InjectableTy>(instance->value());
        }

        /// <summary>
        /// Loads a transient service from storage.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] static auto load_transient_service(
            move_only_any& service,
            typename InjectableTy::descriptor_type::scope_type& scope) -> result<InjectableTy>
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using service_type = typename descriptor_type::service_type;
            using value_type = typename descriptor_type::value_type;

            auto descriptor = service.cast<descriptor_type>();
            if (!descriptor) [[unlikely]]
            {
                DIPP_RETURN_ERROR(incompatible_service_descriptor::error<service_type>());
            }

            auto loaded_instance = descriptor->value().load(scope);
#ifdef DIPP_USE_RESULT
            if (loaded_instance.has_error()) [[unlikely]]
            {
                return loaded_instance.error();
            }
#endif

            auto instance = loaded_instance.cast<value_type>();
            if (!instance || instance->has_error()) [[unlikely]]
            {
#ifdef DIPP_USE_RESULT
                if (instance)
                {
                    return instance->error();
                }
#endif
                DIPP_RETURN_ERROR(mismatched_service_type::error<service_type>());
            }

            return make_result<InjectableTy>(std::move(instance->value()));
        }

    private:
        service_map_type m_Descriptors;
    };
}