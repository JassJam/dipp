#include <benchmark/benchmark.h>
#include <fruit/fruit.h>
#include <kangaru/kangaru.hpp>
#include <dipp/dipp.hpp>
#include <memory>
#include <string>

// Common interfaces and classes
struct ILogger
{
    virtual ~ILogger() = default;
    virtual void log(const std::string& message) = 0;
};

struct IDatabase
{
    virtual ~IDatabase() = default;
    virtual void query(const std::string& sql) = 0;
};

struct IUserService
{
    virtual ~IUserService() = default;
    virtual void createUser(const std::string& username) = 0;
};

// Concrete implementations
class ConsoleLogger : public ILogger
{
public:
    INJECT(ConsoleLogger())
    {
    }
    void log(const std::string& message) override
    {
        benchmark::DoNotOptimize(message);
    }
};

class SQLDatabase : public IDatabase
{
public:
    INJECT(SQLDatabase())
    {
    }
    void query(const std::string& sql) override
    {
        benchmark::DoNotOptimize(sql);
    }
};

class UserService : public IUserService
{
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IDatabase> database_;

public:
    INJECT(UserService(std::shared_ptr<ILogger> logger, std::shared_ptr<IDatabase> database))
        : logger_(std::move(logger))
        , database_(std::move(database))
    {
    }

    void createUser(const std::string& username) override
    {
        logger_->log("Creating user: " + username);
        database_->query("INSERT INTO users (username) VALUES ('" + username + "')");
    }
};

// Fruit Configuration
class LoggerComponent
{
public:
    fruit::Component<ILogger> static getComponent()
    {
        return fruit::createComponent().bind<ILogger, ConsoleLogger>();
    }
};

class DatabaseComponent
{
public:
    fruit::Component<IDatabase> static getComponent()
    {
        return fruit::createComponent().bind<IDatabase, SQLDatabase>();
    }
};

class UserServiceComponent
{
public:
    fruit::Component<IUserService> static getComponent()
    {
        return fruit::createComponent()
            .install(LoggerComponent::getComponent)
            .install(DatabaseComponent::getComponent)
            .bind<IUserService, UserService>();
    }
};

// Kangaru Configuration
struct ConsoleLoggerService;
struct ILoggerService
    : kgr::abstract_shared_service<ILogger>
    , kgr::defaults_to<ConsoleLoggerService>
{
};
struct ConsoleLoggerService
    : kgr::shared_service<ConsoleLogger>
    , kgr::overrides<ILoggerService>
{
};

struct SQLDatabaseService;
struct IDatabaseService
    : kgr::abstract_shared_service<IDatabase>
    , kgr::defaults_to<SQLDatabaseService>
{
};
struct SQLDatabaseService
    : kgr::shared_service<SQLDatabase>
    , kgr::overrides<IDatabaseService>
{
};

struct UserServiceService;
struct IUserServiceService
    : kgr::abstract_shared_service<IUserService>
    , kgr::defaults_to<UserServiceService>
{
};
struct UserServiceService
    : kgr::shared_service<UserService, kgr::dependency<ILoggerService, IDatabaseService>>
    , kgr::overrides<UserServiceService, IUserServiceService>
{
};

// Dipp Configuration
using DippLoggerService = dipp::injected_shared<ILogger, dipp::service_lifetime::singleton>;
struct DippLoggerServiceConfig
{
    static void setup(dipp::default_service_collection& collection)
    {
        collection.add_impl<DippLoggerService, ConsoleLogger>();
    }
};

using DippDatabaseService = dipp::injected_shared<IDatabase, dipp::service_lifetime::singleton>;
struct DippDatabaseServiceConfig
{
    static void setup(dipp::default_service_collection& collection)
    {
        collection.add_impl<DippDatabaseService, SQLDatabase>();
    }
};

using DippUserServiceService =
    dipp::injected_shared<IUserService,
                          dipp::service_lifetime::singleton,
                          dipp::dependency<DippLoggerService, DippDatabaseService>>;
struct DippUserServiceServiceConfig
{
    static void setup(dipp::default_service_collection& collection)
    {
        collection.add_impl<DippUserServiceService, UserService>();
    }
};

struct DippConfiguration
{
    static dipp::default_service_provider setup()
    {
        dipp::default_service_collection collection;

        DippLoggerServiceConfig::setup(collection);
        DippDatabaseServiceConfig::setup(collection);
        DippUserServiceServiceConfig::setup(collection);

        return dipp::default_service_provider(std::move(collection));
    }
};

// Benchmarks for construction and usage

// Fruit Benchmarks
static void BM_FruitContainerCreation(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto injector = fruit::Injector(UserServiceComponent::getComponent);
        benchmark::DoNotOptimize(injector);
    }
}
BENCHMARK(BM_FruitContainerCreation);

static void BM_FruitResolution(benchmark::State& state)
{
    state.PauseTiming();
    auto injector = fruit::Injector(UserServiceComponent::getComponent);
    state.ResumeTiming();

    for (auto _ : state)
    {
        IUserService* service = injector.get<IUserService*>();
        benchmark::DoNotOptimize(service);
    }
}
BENCHMARK(BM_FruitResolution);

// Kangaru Benchmarks
static void BM_KangaruContainerCreation(benchmark::State& state)
{
    for (auto _ : state)
    {
        kgr::container container;
        container.emplace<UserServiceService>();
    }
}
BENCHMARK(BM_KangaruContainerCreation);

static void BM_KangaruResolution(benchmark::State& state)
{
    state.PauseTiming();
    kgr::container container;
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto service = container.service<IUserServiceService>();
        benchmark::DoNotOptimize(service);
    }
}
BENCHMARK(BM_KangaruResolution);

// Dipp Benchmarks
static void BM_DippContainerCreation(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto services = DippConfiguration::setup();
        benchmark::DoNotOptimize(services);
    }
}
BENCHMARK(BM_DippContainerCreation);

static void BM_DippResolution(benchmark::State& state)
{
    state.PauseTiming();
    auto services = DippConfiguration::setup();
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto service = services.get<DippUserServiceService>();
        benchmark::DoNotOptimize(service);
    }
}
BENCHMARK(BM_DippResolution);

BENCHMARK_MAIN();