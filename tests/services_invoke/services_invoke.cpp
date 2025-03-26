#define BOOST_TEST_MODULE ServicesInvoke_Test
#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(ServiceInvocationTests)

struct Window
{
    int width;
    int height;
    explicit Window(int w = 800, int h = 600)
        : width(w)
        , height(h)
    {
    }
};

using WindowService = dipp::injected<Window, dipp::service_lifetime::singleton>;

BOOST_AUTO_TEST_CASE(GivenWindowService_WhenInvokedWithDifferentWindows_ThenValidatesCorrectly)
{
    // Given
    const Window expectedWindow{1024, 768};
    const Window nonMatchingWindow{1920, 1080};

    dipp::default_service_collection services;
    services.add<WindowService>({[&](auto&) { return expectedWindow; }});

    // When
    dipp::default_service_provider provider(std::move(services));

    // Then
    BOOST_TEST_CONTEXT("Should validate matching window properties")
    {
        dipp::invoke(
            provider,
            +[](WindowService service, const Window& expected)
            {
                BOOST_TEST_CONTEXT("Checking width and height match")
                {
                    BOOST_CHECK_EQUAL(service->width, expected.width);
                    BOOST_CHECK_EQUAL(service->height, expected.height);
                }
            },
            expectedWindow);
    }

    BOOST_TEST_CONTEXT("Should detect non-matching window properties")
    {
        dipp::invoke(
            provider,
            +[](WindowService service, const Window& unexpected)
            {
                BOOST_TEST_CONTEXT("Checking width mismatch")
                {
                    BOOST_CHECK_NE(service->width, unexpected.width);
                }
                BOOST_TEST_CONTEXT("Checking height mismatch")
                {
                    BOOST_CHECK_NE(service->height, unexpected.height);
                }
            },
            nonMatchingWindow);
    }

    BOOST_TEST_CONTEXT("Should validate individual property matches")
    {
        dipp::invoke(
            provider,
            +[](WindowService service, const Window& expected)
            { BOOST_CHECK_EQUAL(service->width, expected.width); },
            expectedWindow);
    }
}

BOOST_AUTO_TEST_SUITE_END()