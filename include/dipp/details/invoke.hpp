#pragma once

#include "scope.hpp"

namespace dipp
{
    namespace details
    {
        template<size_t I,
                 typename Tuple1,
                 typename Tuple2,
                 bool = (I < std::tuple_size_v<Tuple1> && I < std::tuple_size_v<Tuple2>)>
        struct tuple_index_of_first_not_equal_impl
        {
            static constexpr int value =
                std::is_same_v<std::tuple_element_t<I, Tuple1>, std::tuple_element_t<I, Tuple2>>
                    ? tuple_index_of_first_not_equal_impl<I + 1, Tuple1, Tuple2>::value
                    : I;
        };

        template<size_t I, typename Tuple1, typename Tuple2>
        struct tuple_index_of_first_not_equal_impl<I, Tuple1, Tuple2, false>
        {
            static constexpr int value = -1; // All elements are equal or end of tuple reached
        };

        template<typename Tuple1, typename Tuple2>
        struct tuple_index_of_first_not_equal
        {
            static constexpr int value =
                tuple_index_of_first_not_equal_impl<0, Tuple1, Tuple2>::value;
        };

        template<typename FnArgsTy,
                 typename ScopeTy,
                 typename FnTy,
                 size_t... Is,
                 typename... ArgsTy>
        auto invoke_impl(ScopeTy& scope, FnTy&& fn, std::index_sequence<Is...>, ArgsTy&&... args)
        {
            return fn(scope.template get<std::tuple_element_t<Is, FnArgsTy>>()...,
                      std::forward<ArgsTy>(args)...);
        }
    } // namespace details

    /// <summary>
    /// Invoke the function with the given scope and arguments
    /// The function will be invoked with injected arguments if there are any then the rest of the
    /// arguments provided
    /// </summary>
    template<typename ScopeTy, typename FnTy, typename... ArgsTy>
    auto invoke(ScopeTy& scope, FnTy&& fn, ArgsTy&&... args)
    {
        using arg_types = typename function_descriptor<FnTy>::args_types;

        static constexpr size_t args_count = sizeof...(ArgsTy);
        static constexpr size_t arg_types_count = std::tuple_size_v<arg_types>;

        // if arg_types count is zero, then we can directly call the function
        if constexpr (args_count == 0)
        {
            return fn();
        }
        // if args count equals to arg_types count, then we can directly call the function
        else if constexpr (arg_types_count == args_count)
        {
            return fn(std::forward<ArgsTy>(args)...);
        }
        // fetch 0..N where each arg from arg_types[i] is not equal to ArgsTy
        else
        {
            static constexpr size_t index =
                details::tuple_index_of_first_not_equal<arg_types, std::tuple<ArgsTy...>>::value;

            // if index is -1, then all elements are equal and we can directly call the function
            if constexpr (index == -1)
            {
                return fn(std::forward<ArgsTy>(args)...);
            }
            else
            {
                return details::invoke_impl<arg_types>(scope,
                                                       std::forward<FnTy>(fn),
                                                       std::make_index_sequence<index + 1> {},
                                                       std::forward<ArgsTy>(args)...);
            }
        }
    }
} // namespace dipp