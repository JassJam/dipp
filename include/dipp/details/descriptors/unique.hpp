#pragma once

#include "functor.hpp"

namespace dipp::details
{
    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    struct unique_service_descriptor
        : functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class =
            functor_service_descriptor<std::unique_ptr<Ty>, Lifetime, ScopeTy, DepsTy>;
        using value_type = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        constexpr unique_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : base_class(std::move(functor))
        {
        }

    public:
        template<typename ImplTy = Ty, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy> && std::derived_from<ImplTy, Ty>)
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            return unique_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::details::apply<DepsTy>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::details::make_any<std::unique_ptr<Ty>>(
                                std::make_unique<ImplTy>(
                                    std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                });
        }

        template<service_descriptor_type DescTy, typename... ArgsTy>
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            using implementation_type = typename DescTy::value_type;
            using implementation_dependency_type = typename DescTy::dependency_type;

            return unique_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::details::apply<implementation_dependency_type>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::details::make_any<std::unique_ptr<Ty>>(
                                std::make_unique<implementation_type>(
                                    std::forward<decltype(params)>(params)...));
                        },
                        std::move(args));
                });
        }
    };
}