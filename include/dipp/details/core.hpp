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

    struct string_hash
    {
        constexpr string_hash() noexcept : value(0)
        {
        }

        constexpr string_hash(std::string_view str) : value(compute_hash(str))
        {
        }

        constexpr operator size_t() const noexcept
        {
            return value;
        }

        size_t value;

    private:
        static constexpr size_t compute_hash(std::string_view str) noexcept
        {
            size_t hash = 0;
            for (char c : str)
            {
                hash = hash * 31 + static_cast<size_t>(c);
            }
            return hash;
        }
    };

    static constexpr size_t key(const char* str) noexcept
    {
        return string_hash(str).value;
    }

    static constexpr size_t key(const std::string_view& str) noexcept
    {
        return string_hash(str).value;
    }

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