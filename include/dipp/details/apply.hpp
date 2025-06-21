#pragma once

#include "concepts.hpp"
#include "dependency.hpp"

namespace dipp::details
{
    /// <summary>
    /// Applies a factory function to a tuple of dependencies and arguments.
    /// </summary>
    template<typename DepsTy, typename ScopeTy, typename FactoryTy, typename ArgsTy>
    [[nodiscard]] static auto apply(ScopeTy& scope, FactoryTy&& factory, ArgsTy&& args)
    {
        using dependencies_type = typename DepsTy::types;
        using args_type = ArgsTy;

        if constexpr (std::tuple_size_v<dependencies_type> == 0)
        {
            return std::apply(std::forward<FactoryTy>(factory), std::forward<ArgsTy>(args));
        }
        else if constexpr (std::tuple_size_v<args_type> == 0)
        {
            auto dependencies = dipp::details::get_tuple_from_scope<DepsTy>(scope);

            using result_type = decltype(std::apply(std::forward<FactoryTy>(factory),
                                                    std::move(dependencies.value())));

#ifdef DIPP_USE_RESULT
            if (dependencies.has_error())
            {
                return move_only_any::make_error(dependencies.error());
            }
#endif

            return std::apply(std::forward<FactoryTy>(factory), std::move(dependencies.value()));
        }
        else
        {
            auto dependencies = dipp::details::get_tuple_from_scope<DepsTy>(scope);

            using result_type = decltype(std::apply(
                std::forward<FactoryTy>(factory),
                std::tuple_cat(std::move(dependencies.value()), std::forward<ArgsTy>(args))));

#ifdef DIPP_USE_RESULT
            if (dependencies.has_error())
            {
                return move_only_any::make_error(dependencies.error());
            }
#endif

            return std::apply(
                std::forward<FactoryTy>(factory),
                std::tuple_cat(std::move(dependencies.value()), std::forward<ArgsTy>(args)));
        }
    }
}