#pragma once

namespace dipp
{
    enum class service_lifetime : unsigned char
    {
        singleton,
        transient,
        scoped
    };
} // namespace dipp