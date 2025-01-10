#pragma once

#include <any>
#include <concepts>
#include <map>
#include <string>
#include <typeindex>
#include <functional>
#include <memory>

namespace dipp
{
    enum class service_lifetime : uint8_t
    {
        singleton,
        transient,
        scoped
    };

    template<size_t N> struct string_literal
    {
        char data[N];

        constexpr string_literal(const char (&str)[N]) noexcept
        {
            for (size_t i = 0; i < N; ++i)
            {
                data[i] = str[i];
            }
        }
        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return N;
        }
        [[nodiscard]] constexpr const char* c_str() const noexcept
        {
            return data;
        }
    };

    template<> struct string_literal<0>
    {
        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return 0;
        }
        [[nodiscard]] constexpr const char* c_str() const noexcept
        {
            return "";
        }
    };

    //

    template<typename Ty>
    concept function_descriptor_type = requires {
        typename Ty::args_types;
        typename Ty::return_type;
    };

    template<typename Ty> struct function_descriptor;

    template<typename RetTy, typename... ArgsTy> struct function_descriptor<RetTy (*)(ArgsTy...)>
    {
        using args_types  = std::tuple<ArgsTy...>;
        using return_type = RetTy;
    };

    template<typename RetTy, typename... ArgsTy> struct function_descriptor<RetTy(ArgsTy...)>
    {
        using args_types  = std::tuple<ArgsTy...>;
        using return_type = RetTy;
    };

    template<typename RetTy, typename... ArgsTy> struct function_descriptor<std::function<RetTy(ArgsTy...)>>
    {
        using args_types  = std::tuple<ArgsTy...>;
        using return_type = RetTy;
    };

    template<typename RetTy, typename... ArgsTy> struct function_descriptor<std::move_only_function<RetTy(ArgsTy...)>>
    {
        using args_types  = std::tuple<ArgsTy...>;
        using return_type = RetTy;
    };

    template<typename FnTy> struct function_descriptor<std::function<FnTy>> : function_descriptor<FnTy>
    {
    };

    //

    template<function_descriptor_type FnTy, size_t Index>
        requires(Index < std::tuple_size_v<typename FnTy::args_types>)
    struct function_args_at
    {
        using type = std::tuple_element_t<Index, typename FnTy::args_types>;
    };

    template<function_descriptor_type FnTy> struct function_return_type
    {
        using type = typename FnTy::return_type;
    };
} // namespace dipp
