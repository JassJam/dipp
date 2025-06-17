#define BOOST_TEST_MODULE InterfaceService_Test

#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/debug.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(InterfaceService_Test)

//

class Interface
{
public:
    virtual ~Interface() = default;
};

class SomeClass
{
};

class Implementation : public Interface
{
public:
    Implementation(std::unique_ptr<SomeClass>)
    {
    }
};

//

using InterfaceService = dipp::injected_unique< //
    Interface,
    dipp::service_lifetime::transient>;

using SomeClassService = dipp::injected_unique< //
    SomeClass,
    dipp::service_lifetime::transient>;

using ImplementationService = dipp::injected< //
    Implementation,
    dipp::service_lifetime::transient,
    dipp::dependency<SomeClassService>>;

//

BOOST_AUTO_TEST_CASE(GivenInterface_WhenInstantiated_ThenImplementationIsCreatedCorrectly)
{
    // Given
    dipp::service_collection collection;
    collection.add<SomeClassService>();
    collection.add_impl<InterfaceService, ImplementationService>();

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    BOOST_CHECK(services.get<InterfaceService>().has_value());
}

BOOST_AUTO_TEST_SUITE_END()
