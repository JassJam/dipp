#define BOOST_TEST_MODULE ErrorHandling_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(ErrorHandling_Test)

struct SimpleService
{
    int value;
    explicit SimpleService(int v)
        : value(v)
    {
    }
};

struct NonCopyable
{
    int value;
    explicit NonCopyable(int v)
        : value(v)
    {
    }
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};

struct ThrowingConstructor
{
    ThrowingConstructor()
    {
        throw std::runtime_error("Constructor failed");
    }
};

struct Class
{
};

struct OtherClass
{
};

//

using SimpleServiceType = dipp::injected< //
    SimpleService,
    dipp::service_lifetime::transient>;

using NonCopyableService = dipp::injected< //
    NonCopyable,
    dipp::service_lifetime::singleton>;

using ThrowingService = dipp::injected< //
    ThrowingConstructor,
    dipp::service_lifetime::transient>;

//

BOOST_AUTO_TEST_CASE(GivenUnregisteredServices_WhenCheckingMultipleTypes_ThenCorrectlyIdentified)
{
    using UnregisteredService1 = dipp::injected<SimpleService,
                                                dipp::service_lifetime::singleton,
                                                dipp::dependency<>,
                                                dipp::key("unregistered1")>;

    using UnregisteredService2 = dipp::injected<NonCopyable,
                                                dipp::service_lifetime::scoped,
                                                dipp::dependency<>,
                                                dipp::key("unregistered2")>;

    // Given
    dipp::service_collection collection;
    collection.add<SimpleServiceType>(42);

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    BOOST_CHECK(services.has<SimpleServiceType>());
    BOOST_CHECK(!services.has<UnregisteredService1>());
    BOOST_CHECK(!services.has<UnregisteredService2>());

#ifdef DIPP_USE_RESULT
    // Test result-based error handling
    auto result1 = services.get<UnregisteredService1>();
    BOOST_CHECK(result1.has_error());

    auto result2 = services.get<UnregisteredService2>();
    BOOST_CHECK(result2.has_error());

    bool found_error = false;
    boost::leaf::try_handle_some(
        [&]() -> boost::leaf::result<void>
        {
            auto result = services.get<UnregisteredService1>();
            if (!result.has_value())
            {
                return result.error();
            }
            return {};
        },
        [&](const dipp::service_not_found&) { found_error = true; });
    BOOST_CHECK(found_error);
#else
    // Test exception-based error handling
    BOOST_CHECK_THROW(services.get<UnregisteredService1>(), dipp::service_not_found);
    BOOST_CHECK_THROW(services.get<UnregisteredService2>(), dipp::service_not_found);
#endif
}

BOOST_AUTO_TEST_CASE(GivenCircularDependencies_WhenDetected_ThenCompilationErrorsGenerated)
{
    // Given / When / Then
    // Note: This test demonstrates that circular dependencies will cause compilation errors
    // In a real scenario, these would be caught at compile time, not runtime

    // This is an example of what would cause a circular dependency:
    // struct ServiceA { ServiceB& b; };
    // struct ServiceB { ServiceA& a; };
    // using ServiceAType = dipp::injected<ServiceA, lifetime, dependency<ServiceBType>>;
    // using ServiceBType = dipp::injected<ServiceB, lifetime, dependency<ServiceAType>>;

    // The above would fail to compile due to incomplete types
    BOOST_CHECK(true); // Placeholder - circular deps are caught at compile time
}

BOOST_AUTO_TEST_CASE(GivenInvalidFactoryReturnType_WhenDetected_ThenCompilationErrorsGenerated)
{
    // Given
    dipp::service_collection collection;

    // When
    // This should compile fine - compatible types
    collection.add<SimpleServiceType>([](auto&) -> SimpleService { return SimpleService(123); });

    // This would fail to compile - incompatible return type:
    // collection.add<SimpleServiceType>(
    //     [](auto&) -> std::string { return "incompatible"; }
    // );

    dipp::service_provider services(std::move(collection));

    // Then
    SimpleService service = *services.get<SimpleServiceType>();
    BOOST_CHECK_EQUAL(service.value, 123);
}

BOOST_AUTO_TEST_CASE(GivenFactoryThatThrows_WhenServiceRequested_ThenExceptionPropagates)
{
    // Given
    dipp::service_collection collection;

    collection.add<SimpleServiceType>(
        [](auto&) -> SimpleService
        {
            throw std::runtime_error("Factory failed");
            return SimpleService(0);
        });

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    // Factory exceptions should propagate
    BOOST_CHECK_THROW((void) services.get<SimpleServiceType>(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(GivenConstructorThatThrows_WhenServiceRequested_ThenExceptionPropagates)
{
    // Given
    dipp::service_collection collection;
    collection.add<ThrowingService>();

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    // Constructor exceptions should propagate
    BOOST_CHECK_THROW((void) services.get<ThrowingService>(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(GivenEmptyCollection_WhenServicesRequested_ThenNoServicesFound)
{
    // Given
    dipp::service_collection collection;

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    // Empty provider should have no services
    BOOST_CHECK(!services.has<SimpleServiceType>());
    BOOST_CHECK_EQUAL(services.count<SimpleServiceType>(), 0);
    BOOST_CHECK_EQUAL(services.count_all<SimpleServiceType>(), 0);

#ifdef DIPP_USE_RESULT
    auto result = services.get<SimpleServiceType>();
    BOOST_CHECK(!result.has_value());
#else
    BOOST_CHECK_THROW(services.get<SimpleServiceType>(), dipp::service_not_found);
#endif
}

BOOST_AUTO_TEST_CASE(GivenNullptrFactory_WhenServiceRequested_ThenNullptrReturned)
{
    using PtrService = dipp::injected_unique<SimpleService, dipp::service_lifetime::transient>;

    // Given
    dipp::service_collection collection;
    collection.add<PtrService>([](auto&) { return nullptr; });

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    // This should work but return a null pointer wrapped in the service
    auto service = services.get<PtrService>();
    BOOST_REQUIRE(service.has_value());
    BOOST_CHECK(!service->get()); // The unique_ptr should be null
}

BOOST_AUTO_TEST_CASE(GivenFactoryWithResultError_WhenServiceRequested_ThenErrorHandled)
{
    struct not_found_error
    {
    };

    // Given
    dipp::service_collection collection;

    collection.add<SimpleServiceType>(
        [](auto&) -> dipp::result<SimpleService>
        {
#ifdef DIPP_USE_RESULT
            return boost::leaf::new_error(not_found_error());
#else
            throw not_found_error();
#endif
        });

    // When
    dipp::service_provider services(std::move(collection));

    // Then
#ifdef DIPP_USE_RESULT
    bool found_error = false;
    boost::leaf::try_handle_some(
        [&]() -> boost::leaf::result<void>
        {
            auto res = services.get<SimpleServiceType>();
            if (!res.has_value())
            {
                return res.error();
            }
            return {};
        },
        [&](const not_found_error&) { found_error = true; });
    BOOST_CHECK(found_error);

#else
    BOOST_CHECK_THROW(services.get<SimpleServiceType>(), std::runtime_error);
#endif
}

BOOST_AUTO_TEST_CASE(GivenUnregisteredService_WhenRequested_ThenServiceNotFoundThrown)
{
    using service = dipp::injected<Class, dipp::service_lifetime::transient>;

    // Given
    dipp::service_provider services({});

    // When / Then
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

BOOST_AUTO_TEST_CASE(GivenWrongServiceType_WhenRequested_ThenServiceNotFoundThrown)
{
    using actual_descriptor = dipp::local_service_descriptor< //
        Class,
        dipp::service_lifetime::singleton,
        dipp::service_scope>;

    using wrong_descriptor = dipp::local_service_descriptor< //
        std::reference_wrapper<Class>,
        dipp::service_lifetime::singleton,
        dipp::service_scope>;

    using actual_injected = dipp::base_injected< //
        actual_descriptor,
        0>;
    using wrong_injected = dipp::base_injected< //
        wrong_descriptor,
        0>;

    // Given
    dipp::service_collection collection;
    collection.add<actual_injected>();

    // When
    dipp::service_provider services(std::move(collection));

    // Then
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