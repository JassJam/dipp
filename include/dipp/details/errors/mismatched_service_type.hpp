#pragma once

#include "base_error.hpp"

namespace dipp
{
    class mismatched_service_type final : public details::base_error
    {
    private:
        explicit mismatched_service_type(const char* typeName)
            : details::base_error(typeName)
        {
        }

    public:
        template<typename Ty>
        static auto error()
        {
            return mismatched_service_type(typeid(Ty).name());
        }
    };
} // namespace dipp