#define BOOST_TEST_MODULE AbstractServices_Test

#include <boost/test/included/unit_test.hpp>
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

//

using CameraService = dipp::injected_unique<ICamera, dipp::service_lifetime::transient>;

using PerspectiveCameraService  = dipp::injected_unique<PerspectiveCamera, dipp::service_lifetime::transient>;
using OrthographicCameraService = dipp::injected_unique<OrthographicCamera, dipp::service_lifetime::transient>;

//

BOOST_AUTO_TEST_CASE(PerspectiveCamera_Test)
{
    dipp::default_service_collection collection;

    collection.add<CameraService>([](auto&) { return std::make_unique<PerspectiveCamera>(); });
    collection.add<PerspectiveCameraService>();

    dipp::default_service_provider services(std::move(collection));

    auto camera1 = services.get<CameraService>().detach();
    auto camera2 = services.get<PerspectiveCameraService>().detach();

    BOOST_CHECK_NE(camera1.get(), nullptr);
    BOOST_CHECK_NE(camera2.get(), nullptr);

    BOOST_CHECK_EQUAL(camera1->projection(), 1);
    BOOST_CHECK_EQUAL(camera2->projection(), 1);

    BOOST_CHECK_NE(camera1.get(), camera2.get());
}

BOOST_AUTO_TEST_CASE(OrthographicCamera_Test)
{
    dipp::default_service_collection collection;

    collection.add<CameraService>([](auto&) { return std::make_unique<OrthographicCamera>(); });
    collection.add<OrthographicCameraService>();

    dipp::default_service_provider services(std::move(collection));

    auto camera1 = services.get<CameraService>().detach();
    auto camera2 = services.get<OrthographicCameraService>().detach();

    BOOST_CHECK_NE(camera1.get(), nullptr);
    BOOST_CHECK_NE(camera2.get(), nullptr);

    BOOST_CHECK_EQUAL(camera1->projection(), 2);
    BOOST_CHECK_EQUAL(camera2->projection(), 2);

    BOOST_CHECK_NE(camera1.get(), camera2.get());
}

BOOST_AUTO_TEST_CASE(Camera_Factory)
{
    dipp::default_service_collection collection;
}

BOOST_AUTO_TEST_SUITE_END()
