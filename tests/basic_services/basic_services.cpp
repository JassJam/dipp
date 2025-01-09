#define BOOST_TEST_MODULE BasicServices_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(BasicServices_Test)

//

struct Camera
{
    Camera(int fov = 90) : fov(fov)
    {
    }

    int fov;
};
using CameraService = dipp::injected<Camera, dipp::service_lifetime::transient>;

struct Scene
{
    Scene(CameraService cam, int max_entities = 100) : camera(std::move(cam)), max_entities(max_entities)
    {
    }

    CameraService camera;
    int           max_entities;
};
using SceneService = dipp::injected<Scene, dipp::service_lifetime::singleton, dipp::dependency<CameraService>>;

struct World
{
    SceneService  scene;
    CameraService camera;

    World(SceneService scene, CameraService cam) : scene(std::move(scene)), camera(std::move(cam))
    {
    }
};
using WorldService =
    dipp::injected<World, dipp::service_lifetime::scoped, dipp::dependency<SceneService, CameraService>>;

//

BOOST_AUTO_TEST_CASE(TransientReference)
{
    dipp::default_service_collection collection;

    collection.add<CameraService>();

    dipp::default_service_provider services(std::move(collection));

    auto a = services.get<CameraService>();
    auto b = services.get<CameraService>();

    BOOST_CHECK_EQUAL(a->fov, 90);
    BOOST_CHECK_EQUAL(b->fov, 90);

    BOOST_CHECK_NE(a.ptr(), b.ptr());
}

BOOST_AUTO_TEST_CASE(MixedLifetimes)
{
    dipp::default_service_collection collection;

    collection.add<CameraService>();
    collection.add<SceneService>();
    collection.add<WorldService>();

    dipp::default_service_provider services(std::move(collection));

    auto camera = services.get<CameraService>();
    auto scene  = services.get<SceneService>();
    auto world  = services.get<WorldService>();

    BOOST_CHECK_EQUAL(camera->fov, 90);
    BOOST_CHECK_EQUAL(scene->camera->fov, 90);
    BOOST_CHECK_EQUAL(scene->max_entities, 100);
    BOOST_CHECK_EQUAL(world->scene->camera->fov, 90);
    BOOST_CHECK_EQUAL(world->scene->max_entities, 100);

    BOOST_CHECK_NE(scene->camera.ptr(), camera.ptr());
    BOOST_CHECK_NE(world->scene->camera.ptr(), camera.ptr());

    BOOST_CHECK_EQUAL(scene.ptr(), world->scene.ptr());
}

BOOST_AUTO_TEST_CASE(MixedLifetimes_WithScopes)
{
    dipp::default_service_collection collection;

    collection.add<CameraService>();
    collection.add<SceneService>();
    collection.add<WorldService>();

    dipp::default_service_provider services(std::move(collection));

    auto camera = services.get<CameraService>(); // transient
    auto scene  = services.get<SceneService>();  // singleton
    auto world  = services.get<WorldService>();  // scoped

    auto scope   = services.create_scope();
    auto camera2 = scope.get<CameraService>(); // transient
    auto scene2  = scope.get<SceneService>();  // singleton
    auto world2  = scope.get<WorldService>();  // scoped

    BOOST_CHECK_NE(camera.ptr(), camera2.ptr());
    BOOST_CHECK_EQUAL(scene.ptr(), scene2.ptr());
    BOOST_CHECK_NE(world.ptr(), world2.ptr());

    BOOST_CHECK_EQUAL(world->scene.ptr(), world2->scene.ptr());
    BOOST_CHECK_NE(world->camera.ptr(), world2->camera.ptr());

    BOOST_CHECK_EQUAL(world->scene.ptr(), scene.ptr());
    BOOST_CHECK_EQUAL(world2->scene.ptr(), scene2.ptr());
}

BOOST_AUTO_TEST_SUITE_END()
