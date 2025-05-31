#pragma once

#include <string>
#include <stdexcept>

namespace dipp::details
{
#if DIPP_USE_RESULT
    struct base_error
    {
        constexpr explicit base_error(const char* typeName)
            : TypeName(typeName)
        {
        }

        std::string TypeName;
    };
#else
    struct base_error : public std::runtime_error
    {
        explicit base_error(const char* typeName)
            : std::runtime_error(typeName)
        {
        }
    };
#endif
} // namespace dipp::details