#define BOOST_TEST_MODULE BasicServices_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(BasicServices_Test)

//

struct Camera
{
    explicit Camera(int fov = 90)
        : fov(fov)
    {
    }

    int fov;
};
using CameraService = dipp::injected<Camera, dipp::service_lifetime::transient>;

struct Scene
{
    explicit Scene(Camera cam, int max_entities = 100)
        : camera(std::move(cam))
        , max_entities(max_entities)
    {
    }

    Camera camera;
    int max_entities;
};
using SceneService =
    dipp::injected<Scene, dipp::service_lifetime::singleton, dipp::dependency<CameraService>>;

struct World
{
    Scene& scene;
    Camera camera;

    World(Scene& scene, Camera cam)
        : scene(scene)
        , camera(std::move(cam))
    {
    }
};
using WorldService = dipp::
    injected<World, dipp::service_lifetime::scoped, dipp::dependency<SceneService, CameraService>>;

//

BOOST_AUTO_TEST_CASE(GivenTransientService_WhenRequestedTwice_ThenInstancesDiffer)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<CameraService>();

    // When
    dipp::default_service_provider services(std::move(collection));
    auto cameraA = *services.get<CameraService>();
    auto cameraB = *services.get<CameraService>();

    // Then
    BOOST_TEST_CONTEXT("Should maintain property values")
    {
        BOOST_CHECK_EQUAL(cameraA->fov, 90);
        BOOST_CHECK_EQUAL(cameraB->fov, 90);
    }

    BOOST_TEST_CONTEXT("Should create distinct instances")
    {
        BOOST_CHECK_NE(cameraA.ptr(), cameraB.ptr());
    }
}

BOOST_AUTO_TEST_CASE(GivenExternalServiceReference_WhenResolved_ThenDependenciesCorrect)
{
    // Given
    using scene_service = dipp::injected_ref<Scene, dipp::service_lifetime::singleton>;
    using world_service = dipp::injected<World,
                                         dipp::service_lifetime::scoped,
                                         dipp::dependency<scene_service, CameraService>>;

    Scene externalScene(Camera(89), 200);
    dipp::default_service_collection collection;

    collection.add<CameraService>();
    collection.add<scene_service>({[&externalScene](auto&) -> std::reference_wrapper<Scene>
                                   { return std::ref(externalScene); }});
    collection.add<world_service>();

    // When
    dipp::default_service_provider services(std::move(collection));
    Scene& scene = *services.get<scene_service>();
    World& world = *services.get<world_service>();

    // Then
    BOOST_TEST_CONTEXT("Should resolve external references")
    {
        BOOST_CHECK_EQUAL(scene.camera.fov, 89);
        BOOST_CHECK_EQUAL(scene.max_entities, 200);
        BOOST_CHECK_EQUAL(&scene, &externalScene);
    }

    BOOST_TEST_CONTEXT("Should maintain dependency graph")
    {
        BOOST_CHECK_EQUAL(&world.scene, &scene);
        BOOST_CHECK_EQUAL(world.scene.camera.fov, 89);
    }
}

BOOST_AUTO_TEST_CASE(GivenMixedServiceLifetimes_WhenResolved_ThenDependenciesHonorLifetimes)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<CameraService>();
    collection.add<SceneService>();
    collection.add<WorldService>();

    // When
    dipp::default_service_provider services(std::move(collection));
    auto [camera, scene, world] = std::make_tuple(*services.get<CameraService>(),
                                                  *services.get<SceneService>(),
                                                  *services.get<WorldService>());

    // Then
    BOOST_TEST_CONTEXT("Should resolve property values")
    {
        BOOST_CHECK_EQUAL(camera->fov, 90);
        BOOST_CHECK_EQUAL(scene->camera.fov, 90);
        BOOST_CHECK_EQUAL(scene->max_entities, 100);
    }

    BOOST_TEST_CONTEXT("Should respect lifetime differences")
    {
        BOOST_CHECK_NE(&scene->camera, camera.ptr());
        BOOST_CHECK_NE(&world->scene.camera, camera.ptr());
        BOOST_CHECK_EQUAL(scene.ptr(), &world->scene);
    }
}

BOOST_AUTO_TEST_CASE(GivenScopedServices_WhenCreatingNewScope_ThenInstanceBehaviorMatchesLifetime)
{
    // Given
    dipp::default_service_collection collection;
    collection.add<CameraService>();
    collection.add<SceneService>();
    collection.add<WorldService>();

    dipp::default_service_provider rootServices(std::move(collection));
    auto rootScope = rootServices.create_scope();

    // When
    auto [rootCamera, rootScene, rootWorld] = std::make_tuple(*rootServices.get<CameraService>(),
                                                              *rootServices.get<SceneService>(),
                                                              *rootServices.get<WorldService>());

    auto [scopeCamera, scopeScene, scopeWorld] = std::make_tuple(*rootScope.get<CameraService>(),
                                                                 *rootScope.get<SceneService>(),
                                                                 *rootScope.get<WorldService>());

    // Then
    BOOST_TEST_CONTEXT("Transient services should create new instances")
    {
        BOOST_CHECK_NE(rootCamera.ptr(), scopeCamera.ptr());
    }

    BOOST_TEST_CONTEXT("Singleton services should reuse instances")
    {
        BOOST_CHECK_EQUAL(rootScene.ptr(), scopeScene.ptr());
    }

    BOOST_TEST_CONTEXT("Scoped services should create scope-specific instances")
    {
        BOOST_CHECK_NE(rootWorld.ptr(), scopeWorld.ptr());
        BOOST_CHECK_EQUAL(&rootWorld->scene, &scopeWorld->scene);
        BOOST_CHECK_NE(&rootWorld->camera, &scopeWorld->camera);
    }

    BOOST_TEST_CONTEXT("Should maintain cross-service relationships")
    {
        BOOST_CHECK_EQUAL(&rootWorld->scene, rootScene.ptr());
        BOOST_CHECK_EQUAL(&scopeWorld->scene, scopeScene.ptr());
    }
}

BOOST_AUTO_TEST_SUITE_END()