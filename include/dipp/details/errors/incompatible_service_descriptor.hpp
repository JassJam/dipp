#pragma once

#include "base_error.hpp"

namespace dipp
{
    class incompatible_service_descriptor final : public details::base_error
    {
    private:
        explicit incompatible_service_descriptor(const char* typeName)
            : details::base_error(typeName)
        {
        }

    public:
        template<typename Ty>
        static auto error()
        {
            return incompatible_service_descriptor(typeid(Ty).name());
        }
    };
} // namespace dipp