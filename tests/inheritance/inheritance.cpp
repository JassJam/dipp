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
    Implementation(SomeClass&)
    {
    }
};

//

using InterfaceService = dipp::injected_unique<Interface, dipp::service_lifetime::transient>;

using SomeClassService = dipp::injected_unique<SomeClass, dipp::service_lifetime::singleton>;
using ImplementationService = dipp::
    injected<Implementation, dipp::service_lifetime::transient, dipp::dependency<SomeClassService>>;

//

BOOST_AUTO_TEST_CASE(GivenInterface_WhenInstantiated_ThenImplementationIsCreatedCorrectly)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<SomeClassService>();
    collection.add(
        InterfaceService::descriptor_type::factory<ImplementationService::descriptor_type>());

    // When
    dipp::default_service_provider services(std::move(collection));

    // Then
    auto& value = services.get<InterfaceService>().get();
    (void) value;
}

BOOST_AUTO_TEST_SUITE_END()
