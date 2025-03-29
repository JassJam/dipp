#define BOOST_TEST_MODULE Test_Dependency
#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(DependencyManagementTests)

struct DependencyChain
{
    bool a_destroyed = false;
    bool b_destroyed = false;
    bool c_destroyed = false;

    std::vector<int> values;

    struct A
    {
        DependencyChain& chain;

        A(DependencyChain& chain)
            : chain(chain)
        {
            chain.checkpoint(0);
        }

        ~A()
        {
            chain.checkpoint(1);
            chain.a_destroyed = true;
        }
    };

    struct B
    {
        A& a;
        DependencyChain& chain;

        B(A& a, DependencyChain& chain)
            : a(a)
            , chain(chain)
        {
            chain.checkpoint(2);
        }

        ~B()
        {
            chain.checkpoint(3);
            chain.b_destroyed = true;
        }
    };

    struct C
    {
        A& a;
        B& b;
        DependencyChain& chain;

        C(A& a, B& b, DependencyChain& chain)
            : a(a)
            , b(b)
            , chain(chain)
        {
            chain.checkpoint(4);
        }

        ~C()
        {
            chain.checkpoint(5);
            chain.c_destroyed = true;
        }
    };

    using AService = dipp::injected<A, dipp::service_lifetime::singleton>;
    using BService =
        dipp::injected<B, dipp::service_lifetime::singleton, dipp::dependency<AService>>;
    using CService =
        dipp::injected<C, dipp::service_lifetime::singleton, dipp::dependency<AService, BService>>;

public:
    void checkpoint(int value)
    {
        values.push_back(value);
    }

    dipp::default_service_collection initialize_services()
    {
        dipp::default_service_collection services;

        services.add(AService::descriptor_type::factory<A>(std::ref(*this)));
        services.add(BService::descriptor_type::factory<B>(std::ref(*this)));
        services.add(CService::descriptor_type::factory<C>(std::ref(*this)));

        return services;
    }
};

BOOST_FIXTURE_TEST_CASE(
    GivenSingletonDependencies_WhenServiceProviderDestroyed_ThenDestructionOrderCorrect,
    DependencyChain)
{
    // Given
    auto services = initialize_services();

    // When
    {
        dipp::default_service_provider provider(std::move(services));
        [[maybe_unused]] auto a = provider.get<AService>();
        [[maybe_unused]] auto b = provider.get<BService>();
        [[maybe_unused]] auto c = provider.get<CService>();
    } // Provider destroyed

    // Then
    BOOST_TEST_CONTEXT("Should track construction/destruction order")
    {
        const std::vector<int> expected{0, 2, 4, 5, 3, 1};
        BOOST_TEST(values == expected, boost::test_tools::per_element());
    }

    BOOST_TEST_CONTEXT("All services should be destroyed")
    {
        BOOST_CHECK(a_destroyed);
        BOOST_CHECK(b_destroyed);
        BOOST_CHECK(c_destroyed);
    }
}

BOOST_FIXTURE_TEST_CASE(GivenDependencyChain_WhenResolvingServices_ThenConstructionOrderCorrect,
                        DependencyChain)
{
    // Given
    auto services = initialize_services();

    // When
    dipp::default_service_provider provider(std::move(services));
    [[maybe_unused]] auto c = provider.get<CService>().get();

    // Then
    BOOST_TEST_CONTEXT("Should construct dependencies in order")
    {
        const std::vector<int> expected{0, 2, 4};
        BOOST_TEST(values == expected, boost::test_tools::per_element());
    }

    BOOST_TEST_CONTEXT("Should keep services alive")
    {
        BOOST_CHECK(!a_destroyed);
        BOOST_CHECK(!b_destroyed);
        BOOST_CHECK(!c_destroyed);
    }
}

BOOST_AUTO_TEST_SUITE_END()