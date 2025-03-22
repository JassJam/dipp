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
    Scene(Camera cam, int max_entities = 100) : camera(std::move(cam)), max_entities(max_entities)
    {
    }

    Camera camera;
    int    max_entities;
};
using SceneService = dipp::injected<Scene, dipp::service_lifetime::singleton, dipp::dependency<CameraService>>;

struct World
{
    Scene& scene;
    Camera camera;

    World(std::reference_wrapper<Scene> scene, Camera cam) : scene(scene), camera(std::move(cam))
    {
    }
};
using WorldService =
    dipp::injected<World, dipp::service_lifetime::scoped, dipp::dependency<SceneService, CameraService>>;

//

BOOST_AUTO_TEST_CASE(TransientReference_Test)
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

BOOST_AUTO_TEST_CASE(ExternReference_Test)
{
    using scene_service = dipp::injected<std::reference_wrapper<Scene>, dipp::service_lifetime::singleton>;

    using world_service =
        dipp::injected<World, dipp::service_lifetime::scoped, dipp::dependency<scene_service, CameraService>>;

    Scene scene(Camera(89), 200);

    dipp::default_service_collection collection;

    collection.add<CameraService>();
    collection.add(scene_service::descriptor_type([&scene](auto&) -> std::reference_wrapper<Scene> { return scene; }));
    collection.add<world_service>();

    dipp::default_service_provider services(std::move(collection));

    Scene& a = services.get<scene_service>().get();
    World& b = services.get<world_service>();

    BOOST_CHECK_EQUAL(a.camera.fov, 89);
    BOOST_CHECK_EQUAL(a.max_entities, 200);

    BOOST_CHECK_EQUAL(b.scene.camera.fov, 89);
    BOOST_CHECK_EQUAL(b.scene.max_entities, 200);

    BOOST_CHECK_EQUAL(&a, &b.scene);
    BOOST_CHECK_EQUAL(&a, &scene);
}

BOOST_AUTO_TEST_CASE(MixedLifetimes_Test)
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
    BOOST_CHECK_EQUAL(scene->camera.fov, 90);
    BOOST_CHECK_EQUAL(scene->max_entities, 100);
    BOOST_CHECK_EQUAL(world->scene.camera.fov, 90);
    BOOST_CHECK_EQUAL(world->scene.max_entities, 100);

    BOOST_CHECK_NE(&scene->camera, camera.ptr());
    BOOST_CHECK_NE(&world->scene.camera, camera.ptr());

    BOOST_CHECK_EQUAL(scene.ptr(), &world->scene);
}

BOOST_AUTO_TEST_CASE(MixedLifetimes_WithScopes_Test)
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

    BOOST_CHECK_EQUAL(&world->scene, &world2->scene);
    BOOST_CHECK_NE(&world->camera, &world2->camera);

    BOOST_CHECK_EQUAL(&world->scene, scene.ptr());
    BOOST_CHECK_EQUAL(&world2->scene, scene2.ptr());
}

BOOST_AUTO_TEST_SUITE_END()
