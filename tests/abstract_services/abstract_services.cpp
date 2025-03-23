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

using CameraService = dipp::injected_unique<ICamera, dipp::service_lifetime::transient>;

using PerspectiveCameraService =
    dipp::injected_unique<PerspectiveCamera, dipp::service_lifetime::transient>;
using OrthographicCameraService =
    dipp::injected_unique<OrthographicCamera, dipp::service_lifetime::transient>;

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

const std::vector<CameraTestCase> camera_test_cases {
    {[] { return std::make_unique<OrthographicCamera>(); }, "ShouldCreateOrthographicCamera", 2},
    {[] { return std::make_unique<PerspectiveCamera>(); }, "ShouldCreatePerspectiveCamera", 1}};

BOOST_DATA_TEST_CASE(GivenCamera_WhenAddingToCollection_ThenCameraIsCreated,
                     boost::unit_test::data::make(camera_test_cases),
                     test_case)
{
    // Given
    dipp::default_service_collection collection;

    collection.add(CameraService::descriptor_type([&](auto&) { return test_case.factory(); }));

    // When
    dipp::default_service_provider services(std::move(collection));

    // Then
    auto& camera = services.get<CameraService>().get();

    BOOST_TEST_CONTEXT(test_case.description)
    {
        BOOST_REQUIRE_NE(camera.get(), nullptr);
        BOOST_CHECK_EQUAL(camera->projection(), test_case.expected_projection);
    }
}

//

BOOST_AUTO_TEST_SUITE_END()
