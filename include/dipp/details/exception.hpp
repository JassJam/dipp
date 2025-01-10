#pragma once

#include <stdexcept>
#include <utility>

namespace dipp
{
    namespace details
    {
#if _HAS_CXX23
        [[noreturn]] void unreachable()
        {
            std::unreachable();
        }
#else
        [[noreturn]] void unreachable()
        {
            std::terminate();
        }
#endif

        template<typename ExceptionTy, typename Ty>
            requires std::is_base_of_v<std::exception, ExceptionTy>
        [[noreturn]] void fail()
        {
#ifndef DIPP_NO_EXCEPTIONS
            ExceptionTy::template do_throw<Ty>();
#endif
            unreachable();
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