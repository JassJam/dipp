#define BOOST_TEST_MODULE Integration_Test

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>

BOOST_AUTO_TEST_SUITE(Integration_Test)

// Real-world-like service hierarchy
class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& message) = 0;
    virtual std::vector<std::string> get_logs() const = 0;
};

class ConsoleLogger : public ILogger
{
public:
    void log(const std::string& message) override
    {
        m_Logs.push_back("[CONSOLE] " + message);
    }

    std::vector<std::string> get_logs() const override
    {
        return m_Logs;
    }

private:
    std::vector<std::string> m_Logs;
};

class FileLogger : public ILogger
{
public:
    explicit FileLogger(const std::string& filename)
        : m_Filename(filename)
    {
    }

    void log(const std::string& message) override
    {
        m_Logs.push_back("[FILE:" + m_Filename + "] " + message);
    }

    std::vector<std::string> get_logs() const override
    {
        return m_Logs;
    }

private:
    std::string m_Filename;
    std::vector<std::string> m_Logs;
};

class IDatabase
{
public:
    virtual ~IDatabase() = default;
    virtual void save(const std::string& key, const std::string& data) = 0;
    virtual std::string load(const std::string& key) = 0;
    virtual std::vector<std::string> get_all_data() const = 0;
};

class InMemoryDatabase : public IDatabase
{
public:
    explicit InMemoryDatabase(ILogger& logger)
        : m_Logger(logger)
    {
        m_Logger.log("InMemoryDatabase initialized");
    }

    void save(const std::string& key, const std::string& data) override
    {
        m_Data[key] = data;
        m_Logger.log("Saved data with key: " + key);
    }

    std::string load(const std::string& key) override
    {
        auto it = m_Data.find(key);
        if (it != m_Data.end())
        {
            m_Logger.log("Loaded data for key: " + key);
            return it->second;
        }
        m_Logger.log("Key not found: " + key);
        return "";
    }

    std::vector<std::string> get_all_data() const override
    {
        std::vector<std::string> result;
        for (const auto& pair : m_Data)
        {
            result.push_back(pair.first + "=" + pair.second);
        }
        return result;
    }

private:
    std::map<std::string, std::string> m_Data;
    ILogger& m_Logger;
};

class UserService
{
private:
    IDatabase& m_Database;
    ILogger& m_Logger;

public:
    UserService(IDatabase& db, ILogger& logger)
        : m_Database(db)
        , m_Logger(logger)
    {
        m_Logger.log("UserService initialized");
    }

    void create_user(const std::string& username)
    {
        m_Database.save("user:" + username, username);
        m_Logger.log("Created user: " + username);
    }

    bool has_user(const std::string& username)
    {
        std::string userData = m_Database.load("user:" + username);
        bool exists = !userData.empty();
        m_Logger.log("User exists check for " + username + ": " + (exists ? "true" : "false"));
        return exists;
    }

    std::vector<std::string> get_all_users()
    {
        auto allData = m_Database.get_all_data();
        std::vector<std::string> users;
        for (const auto& data : allData)
        {
            if (data.find("user:") == 0)
            {
                users.push_back(data);
            }
        }
        m_Logger.log("Retrieved " + std::to_string(users.size()) + " users");
        return users;
    }
};

class NotificationService
{
private:
    ILogger& m_Logger;

public:
    explicit NotificationService(ILogger& logger)
        : m_Logger(logger)
    {
        m_Logger.log("NotificationService initialized");
    }

    void send(const std::string& message)
    {
        m_Logger.log("NOTIFICATION: " + message);
    }
};

class ApplicationService
{
private:
    UserService& m_UserService;
    NotificationService& m_NotificationService;
    ILogger& m_Logger;

public:
    ApplicationService(UserService& userSvc, NotificationService& notifSvc, ILogger& logger)
        : m_UserService(userSvc)
        , m_NotificationService(notifSvc)
        , m_Logger(logger)
    {
        m_Logger.log("ApplicationService initialized");
    }

    void register_user(const std::string& username)
    {
        if (!m_UserService.has_user(username))
        {
            m_UserService.create_user(username);
            m_NotificationService.send("Welcome " + username + "!");
            m_Logger.log("User registration completed: " + username);
        }
        else
        {
            m_Logger.log("User already exists: " + username);
        }
    }

    std::vector<std::string> get_logs()
    {
        return m_Logger.get_logs();
    }
};

//

using LoggerService = dipp::injected_unique< //
    ILogger,
    dipp::service_lifetime::singleton>;

using DatabaseService = dipp::injected_unique< //
    IDatabase,
    dipp::service_lifetime::singleton,
    dipp::dependency<LoggerService>>;

using UserServiceType = dipp::injected_unique< //
    UserService,
    dipp::service_lifetime::singleton,
    dipp::dependency<DatabaseService, LoggerService>>;

using NotificationServiceType = dipp::injected_unique< //
    NotificationService,
    dipp::service_lifetime::singleton,
    dipp::dependency<LoggerService>>;

using ApplicationServiceType = dipp::injected_unique< //
    ApplicationService,
    dipp::service_lifetime::scoped,
    dipp::dependency<UserServiceType, NotificationServiceType, LoggerService>>;

//

BOOST_AUTO_TEST_CASE(GivenCompleteApplicationStack_WhenUsersRegistered_ThenSystemWorksCorrectly)
{
    // Given
    dipp::service_collection collection;
    collection.add_impl<LoggerService, ConsoleLogger>();
    collection.add_impl<DatabaseService, InMemoryDatabase>();
    collection.add<UserServiceType>();
    collection.add<NotificationServiceType>();
    collection.add<ApplicationServiceType>();

    // When
    dipp::service_provider services(std::move(collection));
    ApplicationService& app = *services.get<ApplicationServiceType>();

    // Register some users
    app.register_user("alice");
    app.register_user("bob");
    app.register_user("alice"); // Should detect duplicate

    // Then
    auto logs = app.get_logs();

    // Check that all expected log messages are present
    bool foundDatabaseInit = false;
    bool foundUserServiceInit = false;
    bool foundNotificationInit = false;
    bool foundAppInit = false;
    bool foundAliceCreated = false;
    bool foundBobCreated = false;
    bool foundAliceDuplicate = false;

    struct StringAndFlag
    {
        std::string str;
        bool* flag;
    };

    const std::vector<StringAndFlag> logs_validation = //
        {{"InMemoryDatabase initialized", &foundDatabaseInit},
         {"UserService initialized", &foundUserServiceInit},
         {"NotificationService initialized", &foundNotificationInit},
         {"ApplicationService initialized", &foundAppInit},
         {"Created user: alice", &foundAliceCreated},
         {"Created user: bob", &foundBobCreated},
         {"User already exists: alice", &foundAliceDuplicate}};

    for (auto& [str, flag] : logs_validation)
    {
        for (const auto& log : logs)
        {
            if (log.find(str) != log.npos)
            {
                *flag = true;
            }
        }
    }

    BOOST_CHECK(foundDatabaseInit);
    BOOST_CHECK(foundUserServiceInit);
    BOOST_CHECK(foundNotificationInit);
    BOOST_CHECK(foundAppInit);
    BOOST_CHECK(foundAliceCreated);
    BOOST_CHECK(foundBobCreated);
    BOOST_CHECK(foundAliceDuplicate);
}

BOOST_AUTO_TEST_CASE(
    GivenMultipleLoggerConfiguration_WhenLoggingToAll_ThenBothLoggersReceiveMessages)
{
    using ConsoleLoggerService = dipp::injected_unique< //
        ILogger,
        dipp::service_lifetime::singleton,
        dipp::dependency<>,
        dipp::key("console")>;

    using FileLoggerService = dipp::injected_unique< //
        ILogger,
        dipp::service_lifetime::singleton,
        dipp::dependency<>,
        dipp::key("file")>;

    using MultiLoggerApp = dipp::injected< //
        struct MultiLoggerAppImpl,
        dipp::service_lifetime::transient,
        dipp::dependency<ConsoleLoggerService, FileLoggerService>>;

    struct MultiLoggerAppImpl
    {
        ILogger& consoleLogger;
        ILogger& fileLogger;

        MultiLoggerAppImpl(ILogger& console, ILogger& file)
            : consoleLogger(console)
            , fileLogger(file)
        {
        }

        void logToAll(const std::string& message)
        {
            consoleLogger.log(message);
            fileLogger.log(message);
        }
    };

    // Given
    dipp::service_collection collection;
    collection.add<ConsoleLoggerService>([](auto&) { return std::make_unique<ConsoleLogger>(); });
    collection.add<FileLoggerService>([](auto&)
                                      { return std::make_unique<FileLogger>("app.log"); });
    collection.add<MultiLoggerApp>();

    // When
    dipp::service_provider services(std::move(collection));
    MultiLoggerAppImpl app = *services.get<MultiLoggerApp>();
    app.logToAll("Test message");

    // Then
    auto consoleLogs = app.consoleLogger.get_logs();
    auto fileLogs = app.fileLogger.get_logs();

    BOOST_REQUIRE_EQUAL(consoleLogs.size(), 1);
    BOOST_REQUIRE_EQUAL(fileLogs.size(), 1);

    BOOST_CHECK(consoleLogs[0].find("[CONSOLE] Test message") != std::string::npos);
    BOOST_CHECK(fileLogs[0].find("[FILE:app.log] Test message") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(
    GivenScopedApplicationInstances_WhenCreated_ThenSingletonsSharedButScopedSeparate)
{
    // Given
    dipp::service_collection collection;
    collection.add_impl<LoggerService, ConsoleLogger>();
    collection.add_impl<DatabaseService, InMemoryDatabase>();
    collection.add<UserServiceType>();
    collection.add<NotificationServiceType>();
    collection.add<ApplicationServiceType>();

    // When
    dipp::service_provider services(std::move(collection));

    // Create two different scopes with separate application instances
    auto scope1 = services.create_scope();
    auto scope2 = services.create_scope();

    ApplicationService& app1 = *scope1.get<ApplicationServiceType>();
    ApplicationService& app2 = *scope2.get<ApplicationServiceType>();

    UserService& user1 = *scope1.get<UserServiceType>();
    UserService& user2 = *scope2.get<UserServiceType>();

    // Then
    // Applications should be different instances (scoped lifetime)
    BOOST_CHECK_NE(&app1, &app2);

    // But should share the same singletons (logger, database, etc.)
    BOOST_CHECK_EQUAL(&user1, &user2); // Same singleton

    // Operations in one scope should be visible in the other (shared database)
    app1.register_user("shared_user");

    // Both applications share the same logger, so logs should accumulate
    auto logs1 = app1.get_logs();
    auto logs2 = app2.get_logs();

    // Should be the same logger instance
    BOOST_CHECK_EQUAL(logs1.size(), logs2.size());
}

BOOST_AUTO_TEST_CASE(
    GivenConditionalServiceConfiguration_WhenDifferentEnvironments_ThenCorrectLoggersUsed)
{
    enum class LogLevel
    {
        Debug,
        Production
    };

    auto configureServices = [](LogLevel level)
    {
        dipp::service_collection collection;

        if (level == LogLevel::Debug)
        {
            collection.add<LoggerService>([](auto&) { return std::make_unique<ConsoleLogger>(); });
        }
        else
        {
            collection.add<LoggerService>(
                [](auto&) { return std::make_unique<FileLogger>("production.log"); });
        }

        collection.add_impl<DatabaseService, InMemoryDatabase>();
        collection.add<NotificationServiceType>();

        return collection;
    };

    // Given / When / Then
    // Test debug configuration
    {
        auto debugCollection = configureServices(LogLevel::Debug);
        dipp::service_provider debugServices(std::move(debugCollection));

        ILogger& logger = *debugServices.get<LoggerService>();

        logger.log("Debug message");
        auto logs = logger.get_logs();

        BOOST_REQUIRE_EQUAL(logs.size(), 1);
        BOOST_CHECK(logs[0].find("[CONSOLE]") != std::string::npos);
    }

    // Test production configuration
    {
        auto prodCollection = configureServices(LogLevel::Production);
        dipp::service_provider prodServices(std::move(prodCollection));

        ILogger& logger = *prodServices.get<LoggerService>();

        logger.log("Production message");
        auto logs = logger.get_logs();

        BOOST_REQUIRE_EQUAL(logs.size(), 1);
        BOOST_CHECK(logs[0].find("[FILE:production.log]") != std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(GivenServiceReplacement_WhenLastServiceWins_ThenReplacementSuccessful)
{
    // Given
    dipp::service_collection collection;

    // First, add a console logger
    collection.add_impl<LoggerService, ConsoleLogger>();

    // Then, replace it with a file logger (last one wins)
    collection.add<LoggerService>([](auto&)
                                  { return std::make_unique<FileLogger>("replacement.log"); });

    collection.add_impl<DatabaseService, InMemoryDatabase>();
    collection.add<NotificationServiceType>();

    // When
    dipp::service_provider services(std::move(collection));

    NotificationService& notification = *services.get<NotificationServiceType>();
    notification.send("Test replacement");

    ILogger& logger = *services.get<LoggerService>();
    auto logs = logger.get_logs();

    // Then
    // Should show file logger was used (replacement worked)
    bool foundFileLog = false;
    for (const auto& log : logs)
    {
        if (log.find("[FILE:replacement.log]") != std::string::npos)
        {
            foundFileLog = true;
            break;
        }
    }

    BOOST_CHECK(foundFileLog);
}

BOOST_AUTO_TEST_SUITE_END()