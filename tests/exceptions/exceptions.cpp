#define BOOST_TEST_MODULE ServiceExceptions_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(ServiceExceptions_Test)

struct Class
{
};

struct OtherClass
{
};

BOOST_AUTO_TEST_CASE(ServiceNotFoundException_Test)
{
    using service = dipp::injected<Class, dipp::service_lifetime::transient>;

    dipp::default_service_provider services({});

    BOOST_CHECK_EQUAL(services.has<service>(), false);

#ifdef DIPP_USE_RESULT

    bool found_service_not_found_error = false;

    boost::leaf::try_handle_some(
        [&]() -> boost::leaf::result<void>
        {
            auto result = services.get<service>();
            if (!result.has_value())
            {
                return result.error();
            }
            return {};
        },
        [&](const dipp::service_not_found&) { found_service_not_found_error = true; });

    BOOST_CHECK(found_service_not_found_error);

#else

    BOOST_CHECK_THROW((void) services.get<service>(), dipp::service_not_found);

#endif
}

BOOST_AUTO_TEST_CASE(ServiceNotFoundException_WrongType_Test)
{
    using actual_descriptor = dipp::local_service_descriptor<Class,
                                                             dipp::service_lifetime::singleton,
                                                             dipp::default_service_scope>;

    using wrong_descriptor = dipp::local_service_descriptor<std::reference_wrapper<Class>,
                                                            dipp::service_lifetime::singleton,
                                                            dipp::default_service_scope>;

    using actual_injected = dipp::base_injected<actual_descriptor, 0>;
    using wrong_injected = dipp::base_injected<wrong_descriptor, 0>;

    dipp::default_service_collection collection;

    collection.add<actual_injected>();

    dipp::default_service_provider services(std::move(collection));

    BOOST_CHECK_EQUAL(services.has<actual_injected>(), true);
    BOOST_CHECK_EQUAL(services.has<wrong_injected>(), false);

#ifdef DIPP_USE_RESULT

    bool found_service_not_found_error = false;

    boost::leaf::try_handle_some(
        [&]() -> boost::leaf::result<void>
        {
            auto result = services.get<wrong_injected>();
            if (!result.has_value())
            {
                return result.error();
            }
            return {};
        },
        [&](const dipp::service_not_found&) { found_service_not_found_error = true; });

    BOOST_CHECK(found_service_not_found_error);

#else

    BOOST_CHECK_THROW((void) services.get<wrong_injected>(), dipp::service_not_found);

#endif
}

BOOST_AUTO_TEST_SUITE_END()
