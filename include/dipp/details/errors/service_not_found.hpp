#pragma once

#include "base_error.hpp"

namespace dipp
{
    class service_not_found final : public details::base_error
    {
    private:
        explicit service_not_found(const char* typeName)
            : details::base_error(typeName)
        {
        }

    public:
        template<typename Ty>
        static auto error()
        {
            return service_not_found(typeid(Ty).name());
        }
    };
} // namespace dipp