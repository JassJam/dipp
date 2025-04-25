#pragma once

#include "concepts.hpp"

namespace dipp
{
    namespace details
    {
        template<typename Ty>
        struct is_reference_wrapper_t : std::false_type
        {
        };

        template<typename Ty>
        struct is_reference_wrapper_t<std::reference_wrapper<Ty>> : std::true_type
        {
        };
    }

    template<typename Ty>
    inline constexpr bool is_reference_wrapper_v = details::is_reference_wrapper_t<Ty>::value;

    namespace details
    {
        template<typename Ty>
        struct unwrap_descriptor_type;

        template<service_descriptor_type Ty>
        struct unwrap_descriptor_type<Ty>
        {
            using type = Ty;
        };

        template<base_injected_type Ty>
        struct unwrap_descriptor_type<Ty>
        {
            using type = typename Ty::descriptor_type;
        };
    }

    template<typename Ty>
    using unwrap_descriptor_type_t = typename details::unwrap_descriptor_type<Ty>::type;
} // namespace dipp