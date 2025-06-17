#pragma once

#include <functional>
#include "base.hpp"

namespace dipp::details
{
    template<typename Ty,
             service_lifetime Lifetime,
             service_scope_type ScopeTy,
             dependency_container_type DepsTy = dependency<>>
    struct functor_service_descriptor : base_service_descriptor<Ty, Lifetime, DepsTy>
    {
    public:
        using value_type = Ty;
        using scope_type = ScopeTy;
#if _HAS_CXX23
        using functor_type = std::move_only_function<move_only_any(scope_type& scope)>;
#else
        using functor_type = std::function<move_only_any(scope_type& scope)>;
#endif

        static constexpr service_lifetime lifetime = Lifetime;

    public:
        constexpr functor_service_descriptor(functor_type functor) noexcept(
            std::is_nothrow_move_constructible_v<functor_type>)
            : m_Functor(std::move(functor))
        {
        }

    public:
        constexpr move_only_any load(scope_type& scope) noexcept(
            std::is_nothrow_invocable_v<functor_type, scope_type&>)
        {
            return m_Functor(scope);
        }

    private:
        functor_type m_Functor{};
    };
}