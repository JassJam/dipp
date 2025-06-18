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
}