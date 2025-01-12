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

    namespace details
    {

        template<size_t N> struct string_literal
        {
            constexpr string_literal(const char (&str)[N]) noexcept
            {
                for (size_t i = 0; i < N; ++i)
                {
                    value[i] = str[i];
                }
            }
            char value[N];
        };

        template<> struct string_literal<1>
        {
            constexpr string_literal(const char (&)[1]) noexcept
            {
            }
            static constexpr char value[1] = { '\0' };
        };

        template<> struct string_literal<0>
        {
            constexpr string_literal() noexcept = default;
            static constexpr char value[1]      = { '\0' };
        };

    } // namespace details

    template<details::string_literal Str> struct string_hash
    {
    private:
        static constexpr size_t compute_hash()
        {
            size_t hash = 0;
            for (size_t i = 0; Str.value[i] != '\0'; ++i)
            {
                hash = hash * 31 + Str.value[i];
            }
            return hash;
        }

    public:
        static constexpr size_t value = compute_hash();
    };

    template<> struct string_hash<details::string_literal{ "" }>
    {
        static constexpr size_t value = 0;
    };

    template<> struct string_hash<details::string_literal<0>{}>
    {
        static constexpr size_t value = 0;
    };

    using default_string_hash = string_hash<details::string_literal<0>{}>;

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

    template<typename FnTy> struct function_descriptor<std::function<FnTy>> : function_descriptor<FnTy>
    {
    };

#if _HAS_CXX23
    template<typename RetTy, typename... ArgsTy> struct function_descriptor<std::move_only_function<RetTy(ArgsTy...)>>
    {
        using args_types  = std::tuple<ArgsTy...>;
        using return_type = RetTy;
    };

    template<typename FnTy> struct function_descriptor<std::move_only_function<FnTy>> : function_descriptor<FnTy>
    {
    };
#endif

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
