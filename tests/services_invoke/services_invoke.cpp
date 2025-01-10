#define BOOST_TEST_MODULE ServicesInvoke_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>
#include <boost/compat/function_ref.hpp>

BOOST_AUTO_TEST_SUITE(ServicesInvoke_Test)

//

struct Window
{
    int width;
    int height;

    Window(int width = 800, int height = 600) : width(width), height(height)
    {
    }
};
using WindowService = dipp::injected<Window, dipp::service_lifetime::singleton>;

//

void compare_window_equal(WindowService window, const Window& expected)
{
    BOOST_CHECK_EQUAL(window->width, expected.width);
    BOOST_CHECK_EQUAL(window->height, expected.height);
}

void compare_window_not_equal(WindowService window, const Window& expected)
{
    BOOST_CHECK_NE(window->width, expected.width);
    BOOST_CHECK_NE(window->height, expected.height);
}

//

BOOST_AUTO_TEST_CASE(BasicInvoke_Test)
{
    Window windowA(1024, 768);
    Window windowB(1920, 1080);

    //

    dipp::default_service_collection collection;

    collection.add<WindowService>([windowB](auto&) { return windowB; });

    dipp::default_service_provider services(std::move(collection));

    dipp::invoke(services, &compare_window_equal, windowB);
    dipp::invoke(services, &compare_window_not_equal, windowA);
    dipp::invoke(
        services,
        +[](WindowService window, const Window& expected) { BOOST_CHECK_EQUAL(window->width, expected.width); },
        windowB);
}

BOOST_AUTO_TEST_SUITE_END()
