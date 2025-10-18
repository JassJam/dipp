#define BOOST_TEST_MODULE AbstractServices_Test

#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/debug.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(AbstractServices_Test)

//

class ICamera
{
public:
    virtual ~ICamera() = default;

    virtual int projection() = 0;
};

class PerspectiveCamera : public ICamera
{
public:
    int projection() override
    {
        return 1;
    }
};

class OrthographicCamera : public ICamera
{
public:
    int projection() override
    {
        return 2;
    }
};

//

using CameraService = dipp::injected_unique< //
    ICamera,
    dipp::service_lifetime::transient>;

using PerspectiveCameraService = dipp::injected_unique< //
    PerspectiveCamera,
    dipp::service_lifetime::transient>;

using OrthographicCameraService = dipp::injected_unique< //
    OrthographicCamera,
    dipp::service_lifetime::transient>;

//

struct CameraTestCase
{
    std::function<std::unique_ptr<ICamera>()> factory;
    const char* description;
    int expected_projection;
};

std::ostream& operator<<(std::ostream& os, const CameraTestCase& tc)
{
    return os << tc.description;
}

const std::vector<CameraTestCase> camera_test_cases{
    {[] { return std::make_unique<OrthographicCamera>(); }, "ShouldCreateOrthographicCamera", 2},
    {[] { return std::make_unique<PerspectiveCamera>(); }, "ShouldCreatePerspectiveCamera", 1}};

BOOST_DATA_TEST_CASE(GivenCamera_WhenAddingToCollection_ThenCameraIsCreated,
                     boost::unit_test::data::make(camera_test_cases),
                     test_case)
{
    // Given
    dipp::service_collection collection;

    collection.add<CameraService>([&](auto&) { return test_case.factory(); });

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    std::unique_ptr<ICamera> camera = std::move(*services.get<CameraService>());

    BOOST_TEST_CONTEXT(test_case.description)
    {
        BOOST_REQUIRE_NE(camera.get(), nullptr);
        BOOST_CHECK_EQUAL(camera->projection(), test_case.expected_projection);
    }
}

//

BOOST_AUTO_TEST_CASE(GivenCameraServices_WhenAddingToCollection_ThenCamerasAreCreated)
{
    // Given
    dipp::service_collection collection;

    collection.add_impl<CameraService, PerspectiveCamera>();
    collection.add_impl<CameraService, OrthographicCamera>();
    collection.add_impl<CameraService, OrthographicCamera>();

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    auto camera_count = services.count<CameraService>();
    BOOST_CHECK_EQUAL(camera_count, 3);

    services.for_each<CameraService>(
        [&](const dipp::result<CameraService>& cameraService)
        {
            auto& camera = cameraService->get();
            BOOST_CHECK_NE(camera.get(), nullptr);

            if (dynamic_cast<PerspectiveCamera*>(camera.get()))
            {
                BOOST_CHECK_EQUAL(camera->projection(), 1);
            }
            else if (dynamic_cast<OrthographicCamera*>(camera.get()))
            {
                BOOST_CHECK_EQUAL(camera->projection(), 2);
            }
            else
            {
                BOOST_CHECK(false);
            }
        });
}

BOOST_AUTO_TEST_CASE(
    GivenSingletonCameraServices_WhenQueryingFromCollection_ThenCamerasStaysTheSame)
{
    using singleton_service = dipp::injected_unique< //
        ICamera,
        dipp::service_lifetime::singleton>;

    // Given
    dipp::service_collection collection;

    collection.add_impl<singleton_service, PerspectiveCamera>();
    collection.add_impl<singleton_service, OrthographicCamera>();
    collection.add_impl<singleton_service, OrthographicCamera>();

    // When
    dipp::service_provider services(std::move(collection));

    auto fetch_cameras = [&]()
    {
        std::vector<const ICamera*> cameras;
        services.for_each<singleton_service>(
            [&](const dipp::result<singleton_service>& cameraService)
            { cameras.push_back((*cameraService)->get()); });
        return cameras;
    };

    auto first_cameras = fetch_cameras();
    auto second_cameras = fetch_cameras();

    // Then
    BOOST_CHECK_EQUAL(first_cameras.size(), 3);
    BOOST_TEST(first_cameras == second_cameras, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
