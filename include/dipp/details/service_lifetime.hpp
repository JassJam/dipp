#pragma once


namespace dipp
{
    enum class service_lifetime : uint8_t
    {
        singleton,
        transient,
        scoped
    };
} // namespace dipp