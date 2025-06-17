#define BOOST_TEST_MODULE AdvancedFactory_Test

#include <memory>
#include <functional>
#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(AdvancedFactory_Test)

class BaseService
{
public:
    using Ptr = std::unique_ptr<BaseService>;

public:
    virtual ~BaseService() = default;
    virtual int get_value() const = 0;
};

class ConcreteService : public BaseService
{
private:
    int value_;

public:
    explicit ConcreteService(int value)
        : value_(value)
    {
    }
    int get_value() const override
    {
        return value_;
    }
};

class ComplexService
{
private:
    std::string m_Name;
    int m_Multiplier;
    int m_BaseValue;

public:
    ComplexService(const std::string& name, int mult, int base = 42)
        : m_Name(name)
        , m_Multiplier(mult)
        , m_BaseValue(base)
    {
    }

    int get_result() const
    {
        return m_BaseValue * m_Multiplier;
    }
    const std::string& get_name() const
    {
        return m_Name;
    }
};

//

using BaseServiceType = dipp::injected_unique< //
    BaseService,
    dipp::service_lifetime::singleton>;

using ConcreteServiceType = dipp::injected_unique< //
    ConcreteService,
    dipp::service_lifetime::transient>;

using ComplexServiceType = dipp::injected_unique< //
    ComplexService,
    dipp::service_lifetime::scoped>;

//

BOOST_AUTO_TEST_CASE(GivenFactoryReturningResult_WhenServiceRequested_ThenCorrectValueReturned)
{
    // Given
    dipp::service_collection collection;
    collection.add<BaseServiceType>([](auto&) -> dipp::result<BaseService::Ptr>
                                    { return std::make_unique<ConcreteService>(42); });

    // When
    dipp::service_provider services(std::move(collection));
    BaseService& service = *services.get<BaseServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 42);
}

BOOST_AUTO_TEST_CASE(GivenFactoryReturningRawPointer_WhenServiceRequested_ThenCorrectValueReturned)
{
    // Given
    dipp::service_collection collection;
    collection.add<BaseServiceType>([](auto&) -> std::unique_ptr<BaseService>
                                    { return std::make_unique<ConcreteService>(100); });

    // When
    dipp::service_provider services(std::move(collection));
    BaseService& service = *services.get<BaseServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 100);
}

BOOST_AUTO_TEST_CASE(GivenFactoryWithDependencies_WhenServiceRequested_ThenDependenciesResolved)
{
    // Given
    dipp::service_collection collection;
    collection.add<BaseServiceType>([](auto&) { return std::make_unique<ConcreteService>(10); });
    collection.add<ComplexServiceType>("TestService", 5, 10);

    // When
    dipp::service_provider services(std::move(collection));
    ComplexService& complex = *services.get<ComplexServiceType>();

    // Then
    BOOST_CHECK_EQUAL(complex.get_result(), 50); // 10 * 5
    BOOST_CHECK_EQUAL(complex.get_name(), "TestService");
}

BOOST_AUTO_TEST_CASE(
    GivenFactoryWithCapturedConfiguration_WhenServiceRequested_ThenConfigurationApplied)
{
    using ConfigServiceType = dipp::injected_unique< //
        struct ConfigurableService,
        dipp::service_lifetime::singleton>;

    struct ConfigurableService
    {
        std::string config;
        int multiplier;

        explicit ConfigurableService(const std::string& cfg, int mult = 1)
            : config(cfg)
            , multiplier(mult)
        {
        }

        std::string get_config_info() const
        {
            return config + "_x" + std::to_string(multiplier);
        }
    };

    // Given
    std::string environment = "production";
    int scaleFactor = 3;

    dipp::service_collection collection;
    collection.add<ConfigServiceType>(
        [environment, scaleFactor](auto&)
        { return std::make_unique<ConfigurableService>(environment, scaleFactor); });

    // When
    dipp::service_provider services(std::move(collection));
    ConfigurableService& config = *services.get<ConfigServiceType>();

    // Then
    BOOST_CHECK_EQUAL(config.get_config_info(), "production_x3");
}

BOOST_AUTO_TEST_CASE(
    GivenFactoryReturningReferenceWrapper_WhenServiceRequested_ThenCorrectReferenceReturned)
{
    using RefServiceType = dipp::injected_ref< //
        ConcreteService,
        dipp::service_lifetime::singleton>;

    static ConcreteService staticService(999);

    // Given
    dipp::service_collection collection;
    collection.add<RefServiceType>(
        [](auto&) -> dipp::result<std::reference_wrapper<ConcreteService>>
        { return std::ref(staticService); });

    // When
    dipp::service_provider services(std::move(collection));
    ConcreteService& service = *services.get<RefServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 999);
    BOOST_CHECK_EQUAL(&service, &staticService);
}

BOOST_AUTO_TEST_CASE(GivenFactoryWithExternalCapture_WhenServiceRequested_ThenCapturedValueUsed)
{
    // Given
    int capturedValue = 123;

    dipp::service_collection collection;
    collection.add<BaseServiceType>([capturedValue](auto&)
                                    { return std::make_unique<ConcreteService>(capturedValue); });

    // When
    dipp::service_provider services(std::move(collection));
    BaseService& service = *services.get<BaseServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 123);
}

BOOST_AUTO_TEST_CASE(GivenConditionalFactory_WhenServiceRequested_ThenCorrectBranchExecuted)
{
    // Given
    bool useHighValue = true;

    dipp::service_collection collection;
    collection.add<BaseServiceType>(
        [useHighValue](auto&)
        {
            if (useHighValue)
            {
                return std::make_unique<ConcreteService>(1000);
            }
            else
            {
                return std::make_unique<ConcreteService>(1);
            }
        });

    // When
    dipp::service_provider services(std::move(collection));
    BaseService& service = *services.get<BaseServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service.get_value(), 1000);
}

BOOST_AUTO_TEST_CASE(GivenMutableLambdaFactory_WhenSingletonRequested_ThenCounterIncrementsOnce)
{
    // Given
    dipp::service_collection collection;
    collection.add<BaseServiceType>([counter = 0](auto&) mutable
                                    { return std::make_unique<ConcreteService>(++counter); });

    // When
    dipp::service_provider services(std::move(collection));

    // Since it's a singleton, the counter should only increment once
    BaseService& service1 = *services.get<BaseServiceType>();
    BaseService& service2 = *services.get<BaseServiceType>();

    // Then
    BOOST_CHECK_EQUAL(service1.get_value(), 1);
    BOOST_CHECK_EQUAL(&service1, &service2);
}

BOOST_AUTO_TEST_SUITE_END()