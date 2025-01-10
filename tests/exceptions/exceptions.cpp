#define BOOST_TEST_MODULE ServiceExceptions_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(ServiceExceptions_Test)

struct Class
{
};

BOOST_AUTO_TEST_CASE(ServiceNotFoundException_Test)
{
    using service = dipp::injected<Class, dipp::service_lifetime::transient>;

    dipp::default_service_provider services({});

    BOOST_CHECK_EQUAL(services.has<service>(), false);
    BOOST_CHECK_THROW((void)services.get<service>(), dipp::service_not_found);
}

BOOST_AUTO_TEST_CASE(IncompatibleServiceDescriptorsException_Test)
{
    using actual_descriptor =
        dipp::local_service_descriptor<Class, dipp::service_lifetime::singleton, dipp::default_service_scope>;

    using wrong_descriptor =
        dipp::extern_service_descriptor<Class, dipp::service_lifetime::singleton, dipp::default_service_scope>;

    dipp::default_service_collection collection;

    collection.add<actual_descriptor>();

    dipp::default_service_provider services(std::move(collection));

    BOOST_CHECK_EQUAL(services.has<actual_descriptor>(), true);
    BOOST_CHECK_THROW((void)services.get<wrong_descriptor>(), dipp::incompatible_service_descriptor);
}

BOOST_AUTO_TEST_SUITE_END()
