#pragma once

#include "functor.hpp"

namespace dipp::details
{
    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    struct local_service_descriptor : functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>
    {
    public:
        using base_class = functor_service_descriptor<Ty, Lifetime, ScopeTy, DepsTy>;
        using value_type = typename base_class::value_type;
        using functor_type = typename base_class::functor_type;

        constexpr local_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : base_class(std::move(functor))
        {
        }

    public:
        template<typename ImplTy = Ty, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy>)
        [[nodiscard]] static auto factory(ArgsTy&&... args)
        {
            return local_service_descriptor(
                [args = std::make_tuple(std::forward<ArgsTy>(args)...)](ScopeTy& scope) mutable
                {
                    return dipp::details::apply<DepsTy>(
                        scope,
                        [](auto&&... params) mutable
                        {
                            return dipp::details::make_any<Ty>(
                                std::forward<decltype(params)>(params)...);
                        },
                        std::move(args));
                });
        }
    };
}