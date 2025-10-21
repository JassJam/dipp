#pragma once

#include "policy.hpp"
#include "move_only_any.hpp"

#include "result.hpp"
#include "fail.hpp"

#include "errors/service_not_found.hpp"
#include "errors/incompatible_service_descriptor.hpp"
#include "errors/mismatched_service_type.hpp"

namespace dipp::details
{
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
    };
}