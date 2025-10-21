#pragma once

#include "loader.hpp"

namespace dipp::details
{
    template<base_injected_type InjectableTy,
             service_storage_memory_type SingletonMemTy,
             service_storage_memory_type ScopedMemTy>
    class base_service_getter
    {
    public:
        using scope_type = typename InjectableTy::descriptor_type::scope_type;
        using service_loader_type = service_loader<scope_type, SingletonMemTy, ScopedMemTy>;

        using result_type = result<InjectableTy>;

    public:
        base_service_getter(service_loader_type& loader, move_only_any& service) noexcept
            : m_Loader(loader)
            , m_Service(service)
        {
        }

        [[nodiscard]] auto get() -> result_type
        {
            return m_Loader.template load<InjectableTy>(m_Service);
        }

        auto operator()() -> result_type
        {
            return get();
        }

    private:
        service_loader_type& m_Loader;
        move_only_any& m_Service;
    };

    template<base_injected_type InjectableTy>
    using service_getter = base_service_getter<InjectableTy,
                                               default_service_storage_memory_type,
                                               default_service_storage_memory_type>;
}