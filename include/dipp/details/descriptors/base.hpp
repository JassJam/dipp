#pragma once

#include "../apply.hpp"

namespace dipp::details
{
    template<typename Ty, service_lifetime Lifetime, dependency_container_type DepsTy>
    struct base_service_descriptor
    {
    public:
        using service_type = std::conditional_t<Lifetime == service_lifetime::singleton ||
                                                    Lifetime == service_lifetime::scoped,
                                                std::reference_wrapper<std::remove_reference_t<Ty>>,
                                                Ty>;

        using dependency_type = DepsTy;
    };
}