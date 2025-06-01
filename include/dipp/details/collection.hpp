#pragma once

#include "storage.hpp"

namespace dipp
{
    template<service_policy_type StoragePolicyTy>
    class service_collection
    {
        template<service_policy_type, service_storage_memory_type, service_storage_memory_type>
        friend class service_provider;

    public:
        /// <summary>
        /// Adds a service to the collection using the default factory.
        /// Example: collection.add<LoggerService>();
        /// </summary>
        template<base_injected_type InjectableTy, typename... ArgsTy>
        void add(ArgsTy&&... args)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            add(descriptor_type::factory(std::forward<ArgsTy>(args)...), InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection with a specific implementation type.
        /// Example: collection.add_impl<ILogger, FileLogger>("app.log");
        /// Example: collection.add_impl<DatabaseService, PostgreSQLDatabase>("host=localhost",
        /// 5432);
        /// </summary>
        template<base_injected_type InjectableTy, typename ImplTy, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy> && !base_injected_type<ImplTy> &&
                     !service_descriptor_type<ImplTy>)
        void add_impl(ArgsTy&&... args)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            add(descriptor_type::template factory<ImplTy>(std::forward<ArgsTy>(args)...),
                InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection using another service descriptor as implementation.
        /// Example: collection.add_impl<IServiceType, ConcreteServiceDescriptor>();
        /// </summary>
        template<base_injected_type InjectableTy,
                 service_descriptor_type ImplDescTy,
                 typename... ArgsTy>
        void add_impl(ArgsTy&&... args)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            add(descriptor_type::template factory<ImplDescTy>(std::forward<ArgsTy>(args)...),
                InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection using another service descriptor as implementation.
        /// Example: collection.add_impl<IServiceType, ConcreteService>();
        /// </summary>
        template<base_injected_type InjectableTy,
                 base_injected_type ImplInjectableTy,
                 typename... ArgsTy>
        void add_impl(ArgsTy&&... args)
        {
            using impl_descriptor_type = typename ImplInjectableTy::descriptor_type;
            add_impl<InjectableTy, impl_descriptor_type>(std::forward<ArgsTy>(args)...);
        }

        /// <summary>
        /// Adds a service to the collection using a custom factory function.
        /// Examples:
        ///   // Simple factory:
        ///   collection.add<ConfigService>([](auto& scope) {
        ///       return ConfigService{load_from_file()};
        ///   });
        ///
        ///   // Using dependencies:
        ///   collection.add<UserService>([](auto& scope) {
        ///       auto db = scope.template get<DatabaseService>();
        ///       return std::make_unique<UserService>(db.value());
        ///   });
        ///
        ///   // Conditional logic:
        ///   collection.add<CacheService>([](auto& scope) {
        ///       auto config = scope.template get<ConfigService>();
        ///       if (config->use_redis()) {
        ///           return std::make_unique<RedisCache>();
        ///       } else {
        ///           return std::make_unique<MemoryCache>();
        ///       }
        ///   });
        /// </summary>
        template<base_injected_type InjectableTy, typename FactoryTy>
            requires std::invocable<FactoryTy, typename InjectableTy::descriptor_type::scope_type&>
        void add(FactoryTy&& factory)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using scope_type = typename descriptor_type::scope_type;
            using return_type = std::invoke_result_t<FactoryTy, scope_type&>;

            auto wrapped_factory = [factory = std::forward<FactoryTy>(factory)](
                                       scope_type& scope) mutable -> move_only_any
            {
                if constexpr (std::same_as<return_type, move_only_any>)
                {
                    return factory(scope);
                }
                else
                {
                    return make_any<return_type>(factory(scope));
                }
            };

            add(descriptor_type(std::move(wrapped_factory)), InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection using a pre-constructed descriptor.
        /// Example: collection.add<MyService>(MyService::descriptor_type::factory<Impl>());
        /// </summary>
        template<base_injected_type InjectableTy>
        void add(typename InjectableTy::descriptor_type&& descriptor)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            add(std::forward<descriptor_type>(descriptor), InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection using descriptor type directly.
        /// Example: collection.add<MyServiceDescriptor>();
        /// </summary>
        template<service_descriptor_type DescTy>
        void add(size_t key = {})
        {
            add(DescTy::factory(), key);
        }

        /// <summary>
        /// Adds a service to the collection with a specific key.
        /// Example: collection.add(MyDescriptor{}, dipp::key("custom"));
        /// </summary>
        template<service_descriptor_type DescTy>
        void add(DescTy&& descriptor, size_t key = {})
        {
            m_Storage.add_service(std::forward<DescTy>(descriptor), key);
        }

    public:
        /// <summary>
        /// Emplaces a service in the collection if it doesn't already exist.
        /// Returns true if added, false if already exists.
        /// Example: bool added = collection.emplace<LoggerService>();
        /// </summary>
        template<base_injected_type InjectableTy, typename... ArgsTy>
        bool emplace(ArgsTy&&... args)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return emplace(descriptor_type::factory(std::forward<ArgsTy>(args)...),
                           InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service with specific implementation if it doesn't already exist.
        /// Returns true if added, false if already exists.
        /// Example: bool added = collection.emplace_impl<ILogger, FileLogger>("app.log");
        /// </summary>
        template<base_injected_type InjectableTy, typename ImplTy, typename... ArgsTy>
            requires(!std::is_abstract_v<ImplTy> && !base_injected_type<ImplTy> &&
                     !service_descriptor_type<ImplTy>)
        bool emplace_impl(ArgsTy&&... args)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return emplace(descriptor_type::template factory<ImplTy>(std::forward<ArgsTy>(args)...),
                           InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service using another descriptor as implementation if it doesn't already
        /// exist. Returns true if added, false if already exists. Example: bool added =
        /// collection.emplace_impl<IService, ConcreteServiceDescriptor>();
        /// </summary>
        template<base_injected_type InjectableTy,
                 service_descriptor_type ImplDescTy,
                 typename... ArgsTy>
        bool emplace_impl(ArgsTy&&... args)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return emplace(
                descriptor_type::template factory<ImplDescTy>(std::forward<ArgsTy>(args)...),
                InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service using another descriptor as implementation if it doesn't already
        /// exist. Returns true if added, false if already exists. Example: bool added =
        /// collection.emplace_impl<IService, ConcreteService>();
        /// </summary>
        template<base_injected_type InjectableTy,
                 base_injected_type ImplInjectableTy,
                 typename... ArgsTy>
        bool emplace_impl(ArgsTy&&... args)
        {
            using impl_descriptor_type = typename ImplInjectableTy::descriptor_type;
            return emplace_impl<InjectableTy, impl_descriptor_type>(std::forward<ArgsTy>(args)...);
        }

        /// <summary>
        /// Emplaces a service using a custom factory function if it doesn't already exist.
        /// Returns true if added, false if already exists.
        /// Examples:
        ///   // Simple conditional registration:
        ///   bool added = collection.emplace<ConfigService>([](auto& scope) {
        ///       return ConfigService{get_default_config()};
        ///   });
        ///
        ///   // Only add if not already registered:
        ///   if (collection.emplace<CacheService>([](auto& scope) {
        ///       return std::make_unique<MemoryCache>(1000);
        ///   })) {
        ///       std::cout << "Cache service was registered\n";
        ///   }
        /// </summary>
        template<base_injected_type InjectableTy, typename FactoryTy>
            requires std::invocable<FactoryTy, typename InjectableTy::descriptor_type::scope_type&>
        bool emplace(FactoryTy&& factory)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            using scope_type = typename descriptor_type::scope_type;
            using return_type = std::invoke_result_t<FactoryTy, scope_type&>;

            auto wrapped_factory = [factory = std::forward<FactoryTy>(factory)](
                                       scope_type& scope) mutable -> move_only_any
            {
                if constexpr (std::same_as<return_type, move_only_any>)
                {
                    return factory(scope);
                }
                else
                {
                    return make_any<return_type>(factory(scope));
                }
            };

            return emplace(descriptor_type(std::move(wrapped_factory)), InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service using a pre-constructed descriptor if it doesn't already exist.
        /// Returns true if added, false if already exists.
        /// Example: bool added =
        /// collection.emplace<MyService>(MyService::descriptor_type::factory<Impl>());
        /// </summary>
        template<base_injected_type InjectableTy>
        bool emplace(typename InjectableTy::descriptor_type&& descriptor, size_t key = {})
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return emplace(std::forward<descriptor_type>(descriptor), InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service using descriptor type directly if it doesn't already exist.
        /// Returns true if added, false if already exists.
        /// Example: bool added = collection.emplace<MyServiceDescriptor>();
        /// </summary>
        template<service_descriptor_type DescTy>
        bool emplace(size_t key = {})
        {
            return emplace(DescTy::factory(), key);
        }

        /// <summary>
        /// Emplaces a service with a specific key if it doesn't already exist.
        /// Returns true if added, false if already exists.
        /// Example: bool added = collection.emplace(MyDescriptor{}, dipp::key("unique"));
        /// </summary>
        template<service_descriptor_type DescTy>
        bool emplace(DescTy&& descriptor, size_t key = {})
        {
            return m_Storage.emplace_service(std::forward<DescTy>(descriptor), key);
        }

    public:
        /// <summary>
        /// Checks if a specific service instance is present in the collection.
        /// Example: bool exists = collection.has<MyService>(descriptor_instance);
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has(typename InjectableTy::descriptor_type descriptor) const noexcept
        {
            return m_Storage.template has_service<typename InjectableTy::descriptor_type>(
                std::move(descriptor), InjectableTy::key);
        }

        /// <summary>
        /// Checks if a service type is registered in the collection.
        /// Example: if (collection.has<LoggerService>()) { /* do something */ }
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has() const noexcept
        {
            return m_Storage.template has_service<typename InjectableTy::descriptor_type>(
                InjectableTy::key);
        }

        /// <summary>
        /// Checks if a service descriptor type is registered with a specific key.
        /// Example: bool exists = collection.has<MyServiceDescriptor>(dipp::key("custom"));
        /// </summary>
        template<service_descriptor_type DescTy>
        [[nodiscard]] bool has(size_t key = {}) const noexcept
        {
            return m_Storage.template has_service<DescTy>(key);
        }

    private:
        service_storage<StoragePolicyTy> m_Storage;
    };

    using default_service_collection = service_collection<default_service_policy>;
} // namespace dipp