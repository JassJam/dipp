#pragma once

#include <utility>
#include "result.hpp"
#include "errors/base_error.hpp"

namespace dipp::details
{
#if _HAS_CXX23
    [[noreturn]] inline void unreachable()
    {
        std::unreachable();
    }
#else
    [[noreturn]] inline void unreachable()
    {
        std::terminate();
    }
#endif

    template<typename Error, typename Ty>
        requires std::is_base_of_v<base_error, Error>
    [[noreturn]] auto fail()
    {
        return make_error(Error::template error<Ty>());
    }
} // namespace dipp::details