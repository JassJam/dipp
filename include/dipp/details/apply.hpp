#pragma once

#include "concepts.hpp"
#include "dependency.hpp"

namespace dipp
{
    /// <summary>
    /// Applies a factory function to a tuple of dependencies and arguments.
    /// </summary>
    template<typename DepsTy, typename ScopeTy, typename FactoryTy, typename ArgsTy>
    [[nodiscard]] static auto apply(ScopeTy& scope, FactoryTy&& factory, ArgsTy&& args)
    {
        using dependencies_type = typename DepsTy::types;

        if constexpr (std::tuple_size_v<dependencies_type> == 0)
        {
            return std::apply(std::forward<FactoryTy>(factory), std::forward<ArgsTy>(args));
        }
        else
        {
            auto dependencies = dipp::get_tuple_from_scope<ScopeTy, DepsTy>(scope);
            return std::apply(std::forward<FactoryTy>(factory),
                              std::tuple_cat(dependencies, std::forward<ArgsTy>(args)));
        }
    }
} // namespace dipp