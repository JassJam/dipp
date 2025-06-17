#pragma once

namespace dipp::details
{
    enum class service_lifetime : unsigned char
    {
        singleton,
        transient,
        scoped
    };
}