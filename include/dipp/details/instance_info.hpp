#pragma once

#include <concepts>
#include <any>
#include <vector>
#include <map>

#include "concepts.hpp"
#include "type_key_pair.hpp"
#include "move_only_any.hpp"

namespace dipp::details
{
    struct instance_info
    {
        template<service_descriptor_type DescTy, service_scope_type ScopeTy>
        instance_info(DescTy& descriptor, ScopeTy& scope)
            : Instance(descriptor.load(scope))
        {
        }

#ifdef DIPP_USE_RESULT
        [[nodiscard]]
        bool has_error() const noexcept
        {
            return Instance.has_error();
        }

        [[nodiscard]]
        auto error() noexcept
        {
            return Instance.error();
        }
#endif

        template<typename Ty>
        [[nodiscard]]
        auto cast() noexcept
        {
            return Instance.cast<Ty>();
        }

        move_only_any Instance;
    };
}