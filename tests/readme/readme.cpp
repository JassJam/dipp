#define BOOST_TEST_MODULE ReadMe_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>
#include <mutex>

BOOST_AUTO_TEST_SUITE(ReadMe_Test)

//

// We define a normal class with our logic
struct Window
{
    std::mutex x;
};

struct Engine
{
    Window& window; // singleton window

    Engine(Window& window)
        : window(window)
    {
    }
};

struct Engine2
{
    Window& window1; // singleton window
    Window& window2; // singleton window

    Engine2(Window& window1, Window& window2)
        : window1(window1)
        , window2(window2)
    {
    }
};

//

BOOST_AUTO_TEST_CASE(SingletonAndScoped_Test)
{
    // we define the service with the class, lifetime and optionally the scope and key identifier
    // for unique services the service will be injected as a singleton, meaning that it will be
    // created once and shared across all consumers
    using WindowService = dipp::injected<Window, dipp::service_lifetime::singleton>;

    // Similarly, the engine will be injected as a scoped service, meaning that it will be created
    // once per scope
    using EngineService =
        dipp::injected<Engine, dipp::service_lifetime::scoped, dipp::dependency<WindowService>>;

    dipp::default_service_collection collection;

    // add the services to the collection
    collection.add<WindowService>();
    collection.add<EngineService>();

    // create a service provider with the collection
    dipp::default_service_provider services(std::move(collection));

    // get the engine service
    // the engine service will create a window service and inject it into the engine
    // if the scope is at the root level, the engine will be treated as a singleton
    Engine& engine = *services.get<EngineService>();

    // create a scope
    auto scope = services.create_scope();

    // get the engine service from the scope
    // the engine will be destroyed when the scope is destroyed
    Engine& engine2 = *scope.get<EngineService>();

    // get the window service from the scope
    auto window = scope.get<WindowService>();

    // since the window is a singleton, the window from the scope should be the same as the window
    // from the engine
    BOOST_CHECK_EQUAL(&engine.window, &engine2.window);

    // and the engine from the scope should be different from the engine from the root scope
    BOOST_CHECK_NE(&engine, &engine2);
}

BOOST_AUTO_TEST_CASE(TwoDifferentSingletons_Test)
{
    // we define the service with the class, lifetime and optionally the scope and key identifier
    // for unique services the service will be injected as a singleton, meaning that it will be
    // created once and shared across all consumers
    using WindowService1 = dipp::injected<Window, dipp::service_lifetime::singleton>;
    using WindowService2 = dipp::injected<Window,
                                          dipp::service_lifetime::singleton,
                                          dipp::dependency<>,
                                          dipp::key("UNIQUE")>;

    using EngineService = dipp::injected<Engine2,
                                         dipp::service_lifetime::scoped,
                                         dipp::dependency<WindowService1, WindowService2>>;

    // create a collection to hold our services
    dipp::default_service_collection collection;

    // add the services to the collection
    collection.add<WindowService1>();
    collection.add<WindowService2>();
    collection.add<EngineService>();

    // create a service provider with the collection
    dipp::default_service_provider services(std::move(collection));

    // get the engine service
    // the engine service will create a window service and inject it into the engine
    // if the scope is at the root level, the engine will be treated as a singleton
    Engine2& engine = *services.get<EngineService>();

    // get the window service from the engine
    auto& window1 = engine.window1;
    auto& window2 = engine.window2;

    // both window services shouldn't be the same
    BOOST_CHECK_NE(&window1, &window2);
}

BOOST_AUTO_TEST_SUITE_END()
