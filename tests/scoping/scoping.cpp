#define BOOST_TEST_MODULE AdvancedScoping_Test

#include <memory>
#include <vector>
#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

//

class LifecycleTracker
{
public:
    LifecycleTracker()
    {
        RecordedEvents.clear();
        ExpectedEvents.clear();
    }

    static void record(const std::string& event)
    {
        RecordedEvents.emplace_back(event);
    }

    static void expect(const std::string& event)
    {
        ExpectedEvents.emplace_back(event);
    }

    static void validate()
    {
        BOOST_CHECK_EQUAL_COLLECTIONS(RecordedEvents.begin(),
                                      RecordedEvents.end(),
                                      ExpectedEvents.begin(),
                                      ExpectedEvents.end());
    }

private:
    static inline std::vector<std::string> RecordedEvents;
    static inline std::vector<std::string> ExpectedEvents;
};

BOOST_FIXTURE_TEST_SUITE(AdvancedScoping_Test, LifecycleTracker)

class TrackedSingleton
{
private:
    std::string m_Id;

public:
    explicit TrackedSingleton(const std::string& id)
        : m_Id(id)
    {
        LifecycleTracker::record("Singleton[" + m_Id + "] created");
    }

    ~TrackedSingleton()
    {
        LifecycleTracker::record("Singleton[" + m_Id + "] destroyed");
    }

    const std::string& getId() const
    {
        return m_Id;
    }
};

class TrackedScoped
{
private:
    std::string m_Id;
    const TrackedSingleton& m_Singleton;

public:
    TrackedScoped(const TrackedSingleton& singleton, const std::string& id)
        : m_Id(id)
        , m_Singleton(singleton)
    {
        LifecycleTracker::record("Scoped[" + m_Id + "] created with Singleton[" +
                                 m_Singleton.getId() + "]");
    }

    ~TrackedScoped()
    {
        LifecycleTracker::record("Scoped[" + m_Id + "] destroyed");
    }

    const std::string& getId() const
    {
        return m_Id;
    }
    const TrackedSingleton& getSingleton() const
    {
        return m_Singleton;
    }
};

class TrackedTransient
{
private:
    std::string m_Id;
    const TrackedSingleton& m_Singleton;

public:
    using Ptr = std::unique_ptr<TrackedTransient>;

    TrackedTransient(const TrackedSingleton& singleton, const std::string& id)
        : m_Id(id)
        , m_Singleton(singleton)
    {
        LifecycleTracker::record("Transient[" + m_Id + "] created with Singleton[" +
                                 m_Singleton.getId() + "]");
    }

    ~TrackedTransient()
    {
        LifecycleTracker::record("Transient[" + m_Id + "] destroyed");
    }

    const std::string& getId() const
    {
        return m_Id;
    }
    const TrackedSingleton& getSingleton() const
    {
        return m_Singleton;
    }
};

//

using SingletonService = dipp::injected_unique< //
    TrackedSingleton,
    dipp::service_lifetime::singleton>;

using ScopedService = dipp::injected_unique< //
    TrackedScoped,
    dipp::service_lifetime::scoped,
    dipp::dependency<SingletonService>>;

using TransientService = dipp::injected_unique< //
    TrackedTransient,
    dipp::service_lifetime::transient,
    dipp::dependency<SingletonService>>;

//

BOOST_AUTO_TEST_CASE(GivenNestedScopes_WhenCreatedAndDestroyed_ThenCorrectLifetimeManagement)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<SingletonService>("GlobalSingleton");
    collection.add<ScopedService>("ScopedService");
    collection.add<TransientService>("TransientService");

    // When / Then
    {
        dipp::default_service_provider provider(std::move(collection));
        expect("Singleton[GlobalSingleton] created");

        {
            auto scope1 = provider.create_scope();
            TrackedScoped* scoped1 = *scope1.get<ScopedService>();
            expect("Scoped[ScopedService] created with Singleton[GlobalSingleton]");

            {
                auto scope2 = provider.create_scope();
                TrackedScoped* scoped2 = *scope2.get<ScopedService>();
                expect("Scoped[ScopedService] created with Singleton[GlobalSingleton]");

                // Verify singleton is shared
                BOOST_CHECK_EQUAL(&scoped1->getSingleton(), &scoped2->getSingleton());

                // Verify scoped instances are different
                BOOST_CHECK_NE(scoped1, scoped2);

            } // scope2 destroyed
            expect("Scoped[ScopedService] destroyed");

        } // scope1 destroyed
        expect("Scoped[ScopedService] destroyed");

    } // provider destroyed
    expect("Singleton[GlobalSingleton] destroyed");

    validate();
}

BOOST_AUTO_TEST_CASE(
    GivenTransientServices_WhenRequestedInDifferentScopes_ThenDistinctInstancesCreated)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<SingletonService>("SharedSingleton");
    collection.add<TransientService>("TransientInstance");

    dipp::default_service_provider provider(std::move(collection));

    std::vector<const TrackedTransient*> transientInstances;

    // When
    {
        auto scope1 = provider.create_scope();
        TrackedTransient::Ptr transient1a = *scope1.get<TransientService>();
        TrackedTransient::Ptr transient1b = *scope1.get<TransientService>();

        transientInstances.push_back(transient1a.get());
        transientInstances.push_back(transient1b.get());

        // Transient services should be different instances even in same scope
        BOOST_CHECK_NE(transient1a.get(), transient1b.get());

        {
            auto scope2 = provider.create_scope();
            TrackedTransient::Ptr transient2 = *scope2.get<TransientService>();
            transientInstances.push_back(transient2.get());

            // All transient instances should be different
            BOOST_CHECK_NE(transient1a.get(), transient2.get());
            BOOST_CHECK_NE(transient1b.get(), transient2.get());

            // But they should all share the same singleton
            BOOST_CHECK_EQUAL(&transient1a->getSingleton(), &transient2->getSingleton());
        }
    }

    // Then
    // Verify all instances are unique
    std::set uniqueTransients(transientInstances.begin(), transientInstances.end());
    BOOST_CHECK_EQUAL(uniqueTransients.size(), transientInstances.size());
}

BOOST_AUTO_TEST_CASE(GivenIsolatedScopedServices_WhenRequested_ThenProperIsolation)
{
    using IsolatedScoped1 = dipp::injected_unique< //
        TrackedScoped,
        dipp::service_lifetime::scoped,
        dipp::dependency<SingletonService>,
        dipp::key("isolated1")>;

    using IsolatedScoped2 = dipp::injected_unique< //
        TrackedScoped,
        dipp::service_lifetime::scoped,
        dipp::dependency<SingletonService>,
        dipp::key("isolated2")>;

    // Given
    dipp::default_service_collection collection;
    collection.add<SingletonService>("SharedSingleton");
    collection.add<IsolatedScoped1>("IsolatedService1");
    collection.add<IsolatedScoped2>("IsolatedService2");

    // When
    dipp::default_service_provider provider(std::move(collection));

    auto scope1 = provider.create_scope();
    auto scope2 = provider.create_scope();

    TrackedScoped* isolated1_scope1 = *scope1.get<IsolatedScoped1>();
    TrackedScoped* isolated2_scope1 = *scope1.get<IsolatedScoped2>();
    TrackedScoped* isolated1_scope2 = *scope2.get<IsolatedScoped1>();
    TrackedScoped* isolated2_scope2 = *scope2.get<IsolatedScoped2>();

    // Same service type with same key should be same instance within a scope
    TrackedScoped* isolated1_scope1_again = *scope1.get<IsolatedScoped1>();

    // Then
    BOOST_CHECK_EQUAL(isolated1_scope1, isolated1_scope1_again);

    // Different keys should be different instances
    BOOST_CHECK_NE(isolated1_scope1, isolated2_scope1);

    // Same key in different scopes should be different instances
    BOOST_CHECK_NE(isolated1_scope1, isolated1_scope2);

    // All should share the same singleton
    BOOST_CHECK_EQUAL(&isolated1_scope1->getSingleton(), &isolated2_scope1->getSingleton());
    BOOST_CHECK_EQUAL(&isolated1_scope1->getSingleton(), &isolated1_scope2->getSingleton());
    BOOST_CHECK_EQUAL(&isolated1_scope1->getSingleton(), &isolated2_scope2->getSingleton());
}

BOOST_AUTO_TEST_CASE(GivenDeeplyNestedScopes_WhenCreated_ThenCorrectServiceInstantiation)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<SingletonService>("DeepSingleton");
    collection.add<ScopedService>("DeepScoped");

    dipp::default_service_provider provider(std::move(collection));

    const TrackedSingleton* sharedSingleton = nullptr;
    std::vector<const TrackedScoped*> scopedInstances;

    // When
    // Create deeply nested scopes
    auto scope1 = provider.create_scope();
    {
        TrackedScoped* scoped1 = *scope1.get<ScopedService>();
        sharedSingleton = &scoped1->getSingleton();
        scopedInstances.push_back(scoped1);

        auto scope2 = provider.create_scope();
        {
            TrackedScoped* scoped2 = *scope2.get<ScopedService>();
            scopedInstances.push_back(scoped2);

            auto scope3 = provider.create_scope();
            {
                TrackedScoped* scoped3 = *scope3.get<ScopedService>();
                scopedInstances.push_back(scoped3);

                auto scope4 = provider.create_scope();
                {
                    TrackedScoped* scoped4 = *scope4.get<ScopedService>();
                    scopedInstances.push_back(scoped4);

                    // All should share the same singleton
                    BOOST_CHECK_EQUAL(&scoped2->getSingleton(), sharedSingleton);
                    BOOST_CHECK_EQUAL(&scoped3->getSingleton(), sharedSingleton);
                    BOOST_CHECK_EQUAL(&scoped4->getSingleton(), sharedSingleton);
                }
            }
        }
    }

    // Then
    // All scoped instances should be different
    std::set uniqueScopes(scopedInstances.begin(), scopedInstances.end());
    BOOST_CHECK_EQUAL(uniqueScopes.size(), scopedInstances.size());
}

BOOST_AUTO_TEST_CASE(GivenMovableScopes_WhenMoved_ThenScopedInstancesPreserved)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<SingletonService>("MovableSingleton");
    collection.add<ScopedService>("MovableScoped");

    dipp::default_service_provider provider(std::move(collection));

    // When
    // Get service from root scope
    TrackedScoped* rootScoped = *provider.get<ScopedService>();

    // Create a child scope
    auto childScope = provider.create_scope();
    TrackedScoped* childScoped = *childScope.get<ScopedService>();

    // Move the scope
    auto movedScope = std::move(childScope);
    TrackedScoped* movedScoped = *movedScope.get<ScopedService>();

    // Then
    // Root and child should have different scoped instances
    BOOST_CHECK_NE(rootScoped, childScoped);

    // But same singleton
    BOOST_CHECK_EQUAL(&rootScoped->getSingleton(), &childScoped->getSingleton());

    // Moved scope should maintain the same scoped instance
    BOOST_CHECK_EQUAL(childScoped, movedScoped);
}

BOOST_AUTO_TEST_SUITE_END()