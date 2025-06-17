#define BOOST_TEST_MODULE Decayed_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(Decayed_Test)

//

struct Camera
{
    Camera(int fov = 90)
        : fov(fov)
    {
    }

    int fov;
};
using CameraService = dipp::injected< //
    Camera,
    dipp::service_lifetime::singleton>;

struct Scene
{
    Scene(Camera& cam, int max_entities = 100)
        : camera(std::move(cam))
        , max_entities(max_entities)
    {
    }

    Camera camera;
    int max_entities;
};
using SceneService = dipp::injected< //
    Scene,
    dipp::service_lifetime::singleton,
    dipp::dependency<CameraService>>;

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
using WorldService = dipp::injected_unique< //
    World,
    dipp::service_lifetime::singleton,
    dipp::dependency<SceneService, CameraService>>;

//

BOOST_AUTO_TEST_CASE(GivenNonTransientServices_WhenFetched_ThenTheyCanBeDecayed)
{
    // Given
    dipp::service_collection collection;
    collection.add<CameraService>();
    collection.add<SceneService>();
    collection.add<WorldService>();

    dipp::service_provider rootServices(std::move(collection));
    auto rootScope = rootServices.create_scope();

    // When + Then
    Scene& sceneRef = *rootScope.get<SceneService>();
    (void) sceneRef; // can be decayed to Scene&

    Scene* scenePtr = *rootScope.get<SceneService>();
    (void) scenePtr; // can be decayed to Scene*

    const Scene& sceneConstRef = *rootScope.get<SceneService>();
    (void) sceneConstRef; // can be decayed to const Scene&

    const Scene* sceneConstPtr = *rootScope.get<SceneService>();
    (void) sceneConstPtr; // can be decayed to const Scene*

    //

    World& world = *rootScope.get<WorldService>();
    (void) world; // can be decayed to World&

    World* worldPtr = *rootScope.get<WorldService>();
    (void) worldPtr; // can be decayed to World*

    const World& worldConstRef = *rootScope.get<WorldService>();
    (void) worldConstRef; // can be decayed to const World&

    const World* worldConstPtr = *rootScope.get<WorldService>();
    (void) worldConstPtr; // can be decayed to const World*

    const std::unique_ptr<World>& worldConstUniquePtr = *rootScope.get<WorldService>();
    (void) worldConstUniquePtr; // can be decayed to const std::unique_ptr<World>&

    const std::unique_ptr<World>* worldConstUniquePtrPtr = *rootScope.get<WorldService>();
    (void) worldConstUniquePtrPtr; // can be decayed to const std::unique_ptr<World>*
}

BOOST_AUTO_TEST_SUITE_END()