#define BOOST_TEST_MODULE ServiceKeys_Test

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(ServiceKeys_Test)

struct DatabaseConnection
{
    std::string connectionString;
    explicit DatabaseConnection(const std::string& conn)
        : connectionString(conn)
    {
    }
};

struct CacheManager
{
    std::string cacheType;
    explicit CacheManager(const std::string& type)
        : cacheType(type)
    {
    }
};

//

using PrimaryDbService = dipp::injected_unique< //
    DatabaseConnection,
    dipp::service_lifetime::singleton,
    dipp::dependency<>,
    dipp::key("primary")>;

using SecondaryDbService = dipp::injected_unique< //
    DatabaseConnection,
    dipp::service_lifetime::singleton,
    dipp::dependency<>,
    dipp::key("secondary")>;

using ReadOnlyDbService = dipp::injected_unique< //
    DatabaseConnection,
    dipp::service_lifetime::singleton,
    dipp::dependency<>,
    dipp::key("readonly")>;

using RedisCache = dipp::injected_unique< //
    CacheManager,
    dipp::service_lifetime::singleton,
    dipp::dependency<>,
    dipp::key("redis")>;

using MemoryCache = dipp::injected_unique< //
    CacheManager,
    dipp::service_lifetime::singleton,
    dipp::dependency<>,
    dipp::key("memory")>;

//

BOOST_AUTO_TEST_CASE(GivenMultipleServicesWithKeys_WhenRequested_ThenCorrectServicesReturned)
{
    // Given
    dipp::service_collection collection;

    // Add multiple database connections
    collection.add<PrimaryDbService>("postgres://primary:5432/main");
    collection.add<SecondaryDbService>("postgres://secondary:5432/backup");
    collection.add<ReadOnlyDbService>("postgres://readonly:5432/reports");

    // Add multiple cache managers
    collection.add<RedisCache>("redis");
    collection.add<MemoryCache>("memory");

    // When
    dipp::service_provider services(std::move(collection));

    DatabaseConnection& primary = *services.get<PrimaryDbService>();
    DatabaseConnection& secondary = *services.get<SecondaryDbService>();
    DatabaseConnection& readonly = *services.get<ReadOnlyDbService>();
    CacheManager& redis = *services.get<RedisCache>();
    CacheManager& memory = *services.get<MemoryCache>();

    // Then
    BOOST_CHECK_EQUAL(primary.connectionString, "postgres://primary:5432/main");
    BOOST_CHECK_EQUAL(secondary.connectionString, "postgres://secondary:5432/backup");
    BOOST_CHECK_EQUAL(readonly.connectionString, "postgres://readonly:5432/reports");
    BOOST_CHECK_EQUAL(redis.cacheType, "redis");
    BOOST_CHECK_EQUAL(memory.cacheType, "memory");

    // Verify they are different instances
    BOOST_CHECK_NE(&primary, &secondary);
    BOOST_CHECK_NE(&secondary, &readonly);
    BOOST_CHECK_NE(&redis, &memory);
}

BOOST_AUTO_TEST_CASE(GivenRegisteredKeyedServices_WhenCheckingHas_ThenCorrectAvailabilityReported)
{
    // Given
    dipp::service_collection collection;
    collection.add<PrimaryDbService>("primary");
    collection.add<RedisCache>("redis");

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    // Check that specific keyed services exist
    BOOST_CHECK(services.has<PrimaryDbService>());
    BOOST_CHECK(services.has<RedisCache>());

    // Check that non-registered keyed services don't exist
    BOOST_CHECK(!services.has<SecondaryDbService>());
    BOOST_CHECK(!services.has<MemoryCache>());
}

BOOST_AUTO_TEST_CASE(GivenEmplaceWithKeys_WhenDuplicateKeyAdded_ThenFirstValueKept)
{
    // Given
    dipp::service_collection collection;

    // When
    // First emplace should succeed
    bool added1 = collection.emplace<PrimaryDbService>("first");

    // Second emplace with same key should fail
    bool added2 = collection.emplace<PrimaryDbService>("second");

    // Different key should succeed
    bool added3 = collection.emplace<SecondaryDbService>("different");

    dipp::service_provider services(std::move(collection));

    DatabaseConnection& primary = *services.get<PrimaryDbService>();
    DatabaseConnection& secondary = *services.get<SecondaryDbService>();

    // Then
    BOOST_CHECK(added1);
    BOOST_CHECK(!added2);
    BOOST_CHECK(added3);

    // Should have the first value, not the second
    BOOST_CHECK_EQUAL(primary.connectionString, "first");
    BOOST_CHECK_EQUAL(secondary.connectionString, "different");
}

BOOST_AUTO_TEST_CASE(GivenServicesWithKeys_WhenCountingServices_ThenCorrectCountsReturned)
{
    // Given
    dipp::service_collection collection;

    // Add multiple services with same base type but different keys
    collection.add<PrimaryDbService>("primary");
    collection.add<SecondaryDbService>("secondary");
    collection.add<ReadOnlyDbService>("readonly");

    // Add multiple instances with same key (should be allowed with add)
    collection.add<PrimaryDbService>("primary2");
    collection.add<PrimaryDbService>("primary3");

    // When
    dipp::service_provider services(std::move(collection));

    // Then
    // Count services for specific keys
    BOOST_CHECK_EQUAL(services.count<PrimaryDbService>(), 3);
    BOOST_CHECK_EQUAL(services.count<SecondaryDbService>(), 1);
    BOOST_CHECK_EQUAL(services.count<ReadOnlyDbService>(), 1);

    // Count all database services (all keys combined)
    BOOST_CHECK_EQUAL(services.count_all<PrimaryDbService>(), 5); // All DatabaseConnection services
}

BOOST_AUTO_TEST_CASE(
    GivenDependencyWithSpecificKeys_WhenServiceResolved_ThenCorrectDependenciesInjected)
{
    using DataProcessor = dipp::injected< //
        struct DataProcessorImpl,
        dipp::service_lifetime::scoped,
        dipp::dependency<PrimaryDbService, RedisCache>>;

    struct DataProcessorImpl
    {
        DatabaseConnection& primaryDb;
        CacheManager& cache;

        DataProcessorImpl(DatabaseConnection& db, CacheManager& c)
            : primaryDb(db)
            , cache(c)
        {
        }
    };

    // Given
    dipp::service_collection collection;
    collection.add<PrimaryDbService>("primary-connection");
    collection.add<SecondaryDbService>("secondary-connection");
    collection.add<RedisCache>("redis-cache");
    collection.add<MemoryCache>("memory-cache");
    collection.add<DataProcessor>();

    // When
    dipp::service_provider services(std::move(collection));
    DataProcessorImpl& processor = *services.get<DataProcessor>();

    // Then
    BOOST_CHECK_EQUAL(processor.primaryDb.connectionString, "primary-connection");
    BOOST_CHECK_EQUAL(processor.cache.cacheType, "redis-cache");
}

BOOST_AUTO_TEST_CASE(
    GivenMultipleServicesWithKeys_WhenIteratingWithForEach_ThenCorrectServicesProcessed)
{
    // Given
    dipp::service_collection collection;
    collection.add<PrimaryDbService>("conn1");
    collection.add<PrimaryDbService>("conn2");
    collection.add<PrimaryDbService>("conn3");
    collection.add<SecondaryDbService>("backup1");
    collection.add<SecondaryDbService>("backup2");

    // When
    dipp::service_provider services(std::move(collection));

    // Test for_each with specific key
    std::set<std::string> primaryConnections;
    for (const dipp::result<PrimaryDbService>& service : services.get_all<PrimaryDbService>())
    {
        const DatabaseConnection& dbConn = *service;
        primaryConnections.emplace(dbConn.connectionString);
    }

    // Then
    BOOST_CHECK_EQUAL(primaryConnections.size(), 3);
    BOOST_CHECK(primaryConnections.contains("conn1"));
    BOOST_CHECK(primaryConnections.contains("conn2"));
    BOOST_CHECK(primaryConnections.contains("conn3"));
}

BOOST_AUTO_TEST_SUITE_END()