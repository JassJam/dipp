#pragma once

#include <stdexcept>

namespace dipp
{
    namespace details
    {
        template<typename ExceptionTy, typename Ty>
            requires std::is_base_of_v<std::exception, ExceptionTy>
        [[noreturn]] void fail()
        {
#ifndef DIPP_NO_EXCEPTIONS
            ExceptionTy::template do_throw<Ty>();
#endif
            std::unreachable();
        }
    } // namespace details

    //

    class service_not_found : public std::runtime_error
    {
    private:
        service_not_found(const std::string& typeName) : std::runtime_error("Service not found: " + typeName)
        {
        }

    public:
        template<typename Ty> static void do_throw()
        {
            throw service_not_found(typeid(Ty).name());
        }
    };

    class incompatible_service : public std::runtime_error
    {
    private:
        incompatible_service(const std::string& typeName) : std::runtime_error("Incompatible service: " + typeName)
        {
        }

    public:
        using runtime_error::runtime_error;
        template<typename Ty> static void do_throw()
        {
            throw incompatible_service(typeid(Ty).name());
        }
    };

    class incompatible_service_descriptor : public std::runtime_error
    {
    private:
        incompatible_service_descriptor(const std::string& typeName) :
            std::runtime_error("Incompatible service descriptor: " + typeName)
        {
        }

    public:
        using runtime_error::runtime_error;
        template<typename Ty> static void do_throw()
        {
            throw incompatible_service_descriptor(typeid(Ty).name());
        }
    };
} // namespace dipp