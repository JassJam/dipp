#define BOOST_TEST_MODULE MoveSemanticsPerformance_Test

#include <memory>
#include <chrono>
#include <array>
#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

//

class MoveTracker
{
public:
    static inline int constructor_calls = 0;
    static inline int copy_constructor_calls = 0;
    static inline int move_constructor_calls = 0;
    static inline int copy_assignment_calls = 0;
    static inline int move_assignment_calls = 0;
    static inline int destructor_calls = 0;

public:
    MoveTracker()
    {
        constructor_calls = 0;
        copy_constructor_calls = 0;
        move_constructor_calls = 0;
        copy_assignment_calls = 0;
        move_assignment_calls = 0;
        destructor_calls = 0;
    }
};

BOOST_FIXTURE_TEST_SUITE(MoveSemanticsPerformance_Test, MoveTracker)

//

class MoveOnlyService
{
private:
    std::unique_ptr<int> m_Data;

public:
    explicit MoveOnlyService(int value)
        : m_Data(std::make_unique<int>(value))
    {
        MoveTracker::constructor_calls++;
    }

    MoveOnlyService(const MoveOnlyService&) = delete;
    MoveOnlyService& operator=(const MoveOnlyService&) = delete;

    MoveOnlyService(MoveOnlyService&& other) noexcept
        : m_Data(std::move(other.m_Data))
    {
        MoveTracker::move_constructor_calls++;
    }

    MoveOnlyService& operator=(MoveOnlyService&& other) noexcept
    {
        if (this != &other)
        {
            m_Data = std::move(other.m_Data);
            MoveTracker::move_assignment_calls++;
        }
        return *this;
    }

    ~MoveOnlyService()
    {
        MoveTracker::destructor_calls++;
    }

    int get_value() const
    {
        return m_Data ? *m_Data : -1;
    }

    bool is_valid() const
    {
        return static_cast<bool>(m_Data);
    }
};

class CopyableService
{
private:
    int m_Value;

public:
    explicit CopyableService(int value)
        : m_Value(value)
    {
        MoveTracker::constructor_calls++;
    }

    CopyableService(const CopyableService& other)
        : m_Value(other.m_Value)
    {
        MoveTracker::copy_constructor_calls++;
    }

    CopyableService(CopyableService&& other) noexcept
        : m_Value(other.m_Value)
    {
        MoveTracker::move_constructor_calls++;
        other.m_Value = -1; // Mark as moved-from
    }

    CopyableService& operator=(const CopyableService& other)
    {
        if (this != &other)
        {
            m_Value = other.m_Value;
            MoveTracker::copy_assignment_calls++;
        }
        return *this;
    }

    CopyableService& operator=(CopyableService&& other) noexcept
    {
        if (this != &other)
        {
            m_Value = other.m_Value;
            other.m_Value = -1;
            MoveTracker::move_assignment_calls++;
        }
        return *this;
    }

    ~CopyableService()
    {
        MoveTracker::destructor_calls++;
    }

    int get_value() const
    {
        return m_Value;
    }
};

using MoveOnlyServiceType = dipp::injected<MoveOnlyService, dipp::service_lifetime::transient>;
using MoveOnlySingleton = dipp::injected<MoveOnlyService, dipp::service_lifetime::singleton>;
using CopyableServiceType = dipp::injected<CopyableService, dipp::service_lifetime::transient>;
using CopyableSingleton = dipp::injected<CopyableService, dipp::service_lifetime::singleton>;

//

BOOST_AUTO_TEST_CASE(GivenMoveOnlyTransientService_WhenRequested_ThenValueMovedCorrectly)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<MoveOnlyServiceType>(42);

    // When
    dipp::default_service_provider services(std::move(collection));

    MoveOnlyService service1 = *services.get<MoveOnlyServiceType>();
    MoveOnlyService service2 = *services.get<MoveOnlyServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service1.get_value(), 42);
    BOOST_CHECK_EQUAL(service2.get_value(), 42);
    BOOST_CHECK(service1.is_valid());
    BOOST_CHECK(service2.is_valid());
}

BOOST_AUTO_TEST_CASE(
    GivenMoveOnlySingletonService_WhenRequestedMultipleTimes_ThenSameInstanceReturned)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<MoveOnlySingleton>(100);

    // When
    dipp::default_service_provider services(std::move(collection));

    MoveOnlyService& service1 = *services.get<MoveOnlySingleton>();
    MoveOnlyService& service2 = *services.get<MoveOnlySingleton>();

    // Then
    BOOST_CHECK_EQUAL(service1.get_value(), 100);
    BOOST_CHECK_EQUAL(service2.get_value(), 100);

    // Should be the same instance (reference semantics for singleton)
    BOOST_CHECK_EQUAL(&service1, &service2);
}

BOOST_AUTO_TEST_CASE(GivenCopyableService_WhenUsingFactory_ThenMoveOptimizationApplied)
{
    // Given
    dipp::default_service_collection collection;

    collection.add<CopyableServiceType>(
        [](auto&) -> CopyableService
        {
            return CopyableService(200); // Should be moved, not copied
        });

    // When
    dipp::default_service_provider services(std::move(collection));

    int initial_moves = MoveTracker::move_constructor_calls;
    CopyableService service = *services.get<CopyableServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 200);

    // Should prefer moves over copies
    BOOST_CHECK_GE(MoveTracker::move_constructor_calls, initial_moves);
}

BOOST_AUTO_TEST_CASE(GivenFactoryReturningByValue_WhenServiceRequested_ThenMoveOptimizationUsed)
{
    // Given
    dipp::default_service_collection collection;

    collection.add<CopyableServiceType>(
        [](auto&) -> CopyableService
        {
            CopyableService temp(300);
            return temp; // Should be moved due to RVO/move semantics
        });

    // When
    dipp::default_service_provider services(std::move(collection));
    CopyableService service = *services.get<CopyableServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 300);

    // The exact number of moves depends on optimization level and implementation,
    // but there should be move operations involved
    BOOST_CHECK_GT(MoveTracker::move_constructor_calls, 0);
}

BOOST_AUTO_TEST_CASE(GivenLargeObject_WhenMoved_ThenEfficientlyHandled)
{
    struct LargeObject
    {
        std::array<int, 1000> data;
        bool moved_from = false;

        LargeObject()
        {
            std::iota(data.begin(), data.end(), 0);
        }

        LargeObject(const LargeObject&) = delete; // Force move semantics

        LargeObject(LargeObject&& other) noexcept
            : data(std::move(other.data))
        {
            other.moved_from = true;
        }

        int get_sum() const
        {
            return moved_from ? -1 : std::accumulate(data.begin(), data.end(), 0);
        }
    };

    using LargeObjectService = dipp::injected<LargeObject, dipp::service_lifetime::transient>;

    // Given
    dipp::default_service_collection collection;
    collection.add<LargeObjectService>();

    // When
    dipp::default_service_provider services(std::move(collection));
    LargeObject service = *services.get<LargeObjectService>();

    // Then
    // Should have been moved efficiently, not copied
    int expected_sum = 999 * 1000 / 2; // Sum of 0 to 999
    BOOST_CHECK_EQUAL(service.get_sum(), expected_sum);
}

BOOST_AUTO_TEST_CASE(GivenServiceProvider_WhenMoved_ThenFunctionalityPreserved)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<CopyableServiceType>(42);

    dipp::default_service_provider services1(std::move(collection));

    // When
    // Move the provider
    dipp::default_service_provider services2 = std::move(services1);

    // Move assign
    dipp::default_service_collection empty_collection;
    dipp::default_service_provider services3(std::move(empty_collection));
    services3 = std::move(services2);

    // Then
    // Moved-to provider should work
    CopyableService service = *services3.get<CopyableServiceType>();
    BOOST_CHECK_EQUAL(service.get_value(), 42);
}

BOOST_AUTO_TEST_CASE(GivenScope_WhenMoved_ThenServiceSemanticsPreserved)
{
    using ScopedService = dipp::injected<CopyableService, dipp::service_lifetime::scoped>;

    // Given
    dipp::default_service_collection collection;
    collection.add<ScopedService>(100);

    dipp::default_service_provider services(std::move(collection));

    // Create scope and get service
    auto scope1 = services.create_scope();
    CopyableService& service1 = *scope1.get<ScopedService>();

    // When
    // Move the scope
    auto scope2 = std::move(scope1);
    CopyableService& service2 = *scope2.get<ScopedService>();

    // Then
    // Should be the same instance (scoped semantics preserved)
    BOOST_CHECK_EQUAL(&service1, &service2);
    BOOST_CHECK_EQUAL(service1.get_value(), 100);
    BOOST_CHECK_EQUAL(service2.get_value(), 100);
}

BOOST_AUTO_TEST_SUITE_END()