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
        /// Adds a service to the collection.
        /// </summary>
        template<base_injected_type InjectableTy>
        void add()
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            add(descriptor_type::factory(), InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection.
        /// </summary>
        template<base_injected_type InjectableTy>
        void add(typename InjectableTy::descriptor_type&& descriptor)
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            add(std::forward<descriptor_type>(descriptor), InjectableTy::key);
        }

        /// <summary>
        /// Adds a service to the collection.
        /// </summary>
        template<service_descriptor_type DescTy>
        void add(size_t key = {})
        {
            add(DescTy::factory(), key);
        }

        /// <summary>
        /// Adds a service to the collection.
        /// </summary>
        template<service_descriptor_type DescTy>
        void add(DescTy&& descriptor, size_t key = {})
        {
            m_Storage.add_service(std::forward<DescTy>(descriptor), key);
        }

    public:
        /// <summary>
        /// Emplaces a service in the collection.
        /// </summary>
        template<base_injected_type InjectableTy>
        bool emplace()
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return emplace(descriptor_type::factory(), InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service in the collection.
        /// </summary>
        template<base_injected_type InjectableTy>
        bool emplace(typename InjectableTy::descriptor_type&& descriptor, size_t key = {})
        {
            using descriptor_type = typename InjectableTy::descriptor_type;
            return emplace(std::forward<descriptor_type>(descriptor), InjectableTy::key);
        }

        /// <summary>
        /// Emplaces a service in the collection.
        /// </summary>
        template<service_descriptor_type DescTy>
        bool emplace(size_t key = {})
        {
            return emplace(DescTy::factory(), key);
        }

        /// <summary>
        /// Emplaces a service in the collection.
        /// </summary>
        template<service_descriptor_type DescTy>
        bool emplace(DescTy&& descriptor, size_t key = {})
        {
            return m_Storage.emplace_service(std::forward<DescTy>(descriptor), key);
        }

    public:
        /// <summary>
        /// Checks if a service is present in the collection.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has(typename InjectableTy::descriptor_type descriptor) const noexcept
        {
            return m_Storage.template has_service<typename InjectableTy::descriptor_type>(
                std::move(descriptor), InjectableTy::key);
        }

        /// <summary>
        /// Checks if a service is present in the collection.
        /// </summary>
        template<base_injected_type InjectableTy>
        [[nodiscard]] bool has() const noexcept
        {
            return m_Storage.template has_service<typename InjectableTy::descriptor_type>(
                InjectableTy::key);
        }

        /// <summary>
        /// Checks if a service is present in the collection.
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